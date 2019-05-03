//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_UTILITY_HPP_INCLUDED
#define V8PP_UTILITY_HPP_INCLUDED

#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#if __has_include(<string_view>)
#include <string_view>
namespace v8pp {
	template<typename Char, typename Traits = std::char_traits<Char>>
	using basic_string_view = std::basic_string_view<Char, Traits>;
	using string_view = std::string_view;
	using u16string_view = std::u16string_view;
	using wstring_view = std::wstring_view;
} // namespace v8pp
#else
namespace v8pp {

// polyfill for std::string_view from C++17
template<typename Char, typename Traits = std::char_traits<Char>>
class basic_string_view
{
public:
	using value_type = Char;
	using size_type = size_t;
	using const_iterator = Char const*;

	static const size_type npos = ~0;

	basic_string_view(Char const* data, size_t size)
		: data_(data)
		, size_(size)
	{
	}

	template<typename Alloc>
	basic_string_view(std::basic_string<Char, Traits, Alloc> const& str)
		: basic_string_view(str.data(), str.size())
	{
	}

	basic_string_view(Char const* data)
		: basic_string_view(data, data? Traits::length(data) : 0)
	{
	}

	template<size_t N>
	basic_string_view(Char const (&str)[N])
		: basic_string_view(str, N - 1)
	{
	}

	const_iterator begin() const { return data_; }
	const_iterator end() const { return data_ + size_; }

	const_iterator cbegin() const { return data_; }
	const_iterator cend() const { return data_ + size_; }

	Char const* data() const { return data_; }
	size_t size() const { return size_; }
	bool empty() const { return size_ == 0; }

	size_t find(Char ch) const
	{
		Char const* const ptr = Traits::find(data_, size_, ch);
		return ptr ? ptr - data_ : npos;
	}

	basic_string_view substr(size_t pos = 0, size_t count = npos) const
	{
		if (pos > size_) throw std::out_of_range("pos > size");
		return basic_string_view(data_ + pos, std::min(count, size_ - pos));
	}

	operator std::basic_string<Char, Traits>() const
	{
		return std::basic_string<Char, Traits>(data_, size_);
	}

	friend bool operator==(basic_string_view const& lhs, basic_string_view const& rhs)
	{
		return lhs.size_ == rhs.size_
			&& (lhs.data_ == rhs.data_ || Traits::compare(lhs.data_, rhs.data_, lhs.size_) == 0);
	}

	friend bool operator!=(basic_string_view const& lhs, basic_string_view const& rhs)
	{
		return !(lhs == rhs);
	}

	friend std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& os, basic_string_view const& sv)
	{
		return os.write(sv.data_, sv.size_);
	}
private:
	Char const* data_;
	size_t size_;
};

using string_view = basic_string_view<char>;
using u16string_view = basic_string_view<char16_t>;
using wstring_view = basic_string_view<wchar_t>;
} // namespace v8pp {
#endif

namespace v8pp { namespace detail {

template<typename T>
struct tuple_tail;

template<typename Head, typename... Tail>
struct tuple_tail<std::tuple<Head, Tail...>>
{
	using type = std::tuple<Tail...>;
};

/////////////////////////////////////////////////////////////////////////////
//
// Function traits
//
template<typename F>
struct function_traits;

template<typename R, typename ...Args>
struct function_traits<R (Args...)>
{
	using return_type = R;
	using arguments = std::tuple<Args...>;
};

// function pointer
template<typename R, typename ...Args>
struct function_traits<R (*)(Args...)>
	: function_traits<R (Args...)>
{
	using pointer_type = R (*)(Args...);
};

// member function pointer
template<typename C, typename R, typename ...Args>
struct function_traits<R (C::*)(Args...)>
	: function_traits<R (C&, Args...)>
{
	template<typename D = C>
	using pointer_type = R (D::*)(Args...);
};

// const member function pointer
template<typename C, typename R, typename ...Args>
struct function_traits<R (C::*)(Args...) const>
	: function_traits<R (C const&, Args...)>
{
	template<typename D = C>
	using pointer_type = R (D::*)(Args...) const;
};

// volatile member function pointer
template<typename C, typename R, typename ...Args>
struct function_traits<R (C::*)(Args...) volatile>
	: function_traits<R (C volatile&, Args...)>
{
	template<typename D = C>
	using pointer_type = R (D::*)(Args...) volatile;
};

// const volatile member function pointer
template<typename C, typename R, typename ...Args>
struct function_traits<R (C::*)(Args...) const volatile>
	: function_traits<R (C const volatile&, Args...)>
{
	template<typename D = C>
	using pointer_type = R (D::*)(Args...) const volatile;
};

// member object pointer
template<typename C, typename R>
struct function_traits<R (C::*)>
	: function_traits<R (C&)>
{
	template<typename D = C>
	using pointer_type = R (D::*);
};

// const member object pointer
template<typename C, typename R>
struct function_traits<const R (C::*)>
	: function_traits<R (C const&)>
{
	template<typename D = C>
	using pointer_type = const R (D::*);
};

// volatile member object pointer
template<typename C, typename R>
struct function_traits<volatile R (C::*)>
	: function_traits<R (C volatile&)>
{
	template<typename D = C>
	using pointer_type = volatile R (D::*);
};

// const volatile member object pointer
template<typename C, typename R>
struct function_traits<const volatile R (C::*)>
	: function_traits<R (C const volatile&)>
{
	template<typename D = C>
	using pointer_type = const volatile R (D::*);
};

// function object, std::function, lambda
template<typename F>
struct function_traits
{
	static_assert(!std::is_bind_expression<F>::value,
		"std::bind result is not supported yet");
private:
	using callable_traits = function_traits<decltype(&F::operator())>;
public:
	using return_type = typename callable_traits::return_type;
	using arguments = typename tuple_tail<typename callable_traits::arguments>::type;
};

template<typename F>
struct function_traits<F&> : function_traits<F> {};

template<typename F>
struct function_traits<F&&> : function_traits<F> {};

template<typename F>
using is_void_return = std::is_same<void,
	typename function_traits<F>::return_type>;

template<typename F, bool is_class>
struct is_callable_impl
	: std::is_function<typename std::remove_pointer<F>::type>
{
};

template<typename F>
struct is_callable_impl<F, true>
{
private:
	struct fallback { void operator()(); };
	struct derived : F, fallback {};

	template<typename U, U> struct check;

	template<typename>
	static std::true_type test(...);

	template<typename C>
	static std::false_type test(check<void(fallback::*)(), &C::operator()>*);

	using type = decltype(test<derived>(0));
public:
	static const bool value = type::value;
};

template<typename F>
using is_callable = std::integral_constant<bool,
	is_callable_impl<F, std::is_class<F>::value>::value>;

/// Type information for custom RTTI
class type_info
{
public:
	string_view const& name() const { return name_; }
	bool operator==(type_info const& other) const { return name_ == other.name_; }
	bool operator!=(type_info const& other) const { return name_ != other.name_; }
private:
	template<typename T> friend type_info type_id();
	type_info(char const* name, size_t size)
		: name_(name, size)
	{
	}
	string_view name_;
};

/// Get type information for type T
/// The idea is borrowed from https://github.com/Manu343726/ctti
template<typename T>
type_info type_id()
{
#if defined(_MSC_VER) && !defined(__clang__)
	#define V8PP_PRETTY_FUNCTION __FUNCSIG__
	#define V8PP_PRETTY_FUNCTION_PREFIX "class v8pp::detail::type_info __cdecl v8pp::detail::type_id<"
	#define V8PP_PRETTY_FUNCTION_SUFFIX ">(void)"
#elif defined(__clang__) || defined(__GNUC__)
	#define V8PP_PRETTY_FUNCTION __PRETTY_FUNCTION__
	#if !defined(__clang__)
		#define V8PP_PRETTY_FUNCTION_PREFIX "v8pp::detail::type_info v8pp::detail::type_id() [with T = "
	#else
		#define V8PP_PRETTY_FUNCTION_PREFIX "v8pp::detail::type_info v8pp::detail::type_id() [T = "
	#endif
	#define V8PP_PRETTY_FUNCTION_SUFFIX "]"
#else
	#error "Unknown compiler"
#endif

#define V8PP_PRETTY_FUNCTION_LEN (sizeof(V8PP_PRETTY_FUNCTION) - 1)
#define V8PP_PRETTY_FUNCTION_PREFIX_LEN (sizeof(V8PP_PRETTY_FUNCTION_PREFIX) - 1)
#define V8PP_PRETTY_FUNCTION_SUFFIX_LEN (sizeof(V8PP_PRETTY_FUNCTION_SUFFIX) - 1)

	return type_info(V8PP_PRETTY_FUNCTION + V8PP_PRETTY_FUNCTION_PREFIX_LEN,
		V8PP_PRETTY_FUNCTION_LEN - V8PP_PRETTY_FUNCTION_PREFIX_LEN - V8PP_PRETTY_FUNCTION_SUFFIX_LEN);

#undef V8PP_PRETTY_FUNCTION
#undef V8PP_PRETTY_FUNCTION_PREFIX
#undef V8PP_PRETTY_FUNCTION_SUFFFIX
#undef V8PP_PRETTY_FUNCTION_LEN
#undef V8PP_PRETTY_FUNCTION_PREFIX_LEN
#undef V8PP_PRETTY_FUNCTION_SUFFFIX_LEN
}

}} // namespace v8pp::detail

#endif // V8PP_UTILITY_HPP_INCLUDED
