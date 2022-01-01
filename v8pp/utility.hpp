#ifndef V8PP_UTILITY_HPP_INCLUDED
#define V8PP_UTILITY_HPP_INCLUDED

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#if defined(__cpp_lib_string_view) && V8PP_USE_STD_STRING_VIEW
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
	using traits_type = Traits;
	using size_type = size_t;
	using const_iterator = Char const*;

	static const size_type npos = ~size_type{0};

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
		: basic_string_view(data, data ? Traits::length(data) : 0)
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

	size_t find(basic_string_view str) const
	{
		for (const_iterator it = begin(); it != end(); ++it)
		{
			const size_t rest = end() - it;
			if (str.size() > rest) break;
			if (Traits::compare(it, str.data(), str.size()) == 0)
			{
				return it - begin();
			}
		}
		return npos;
	}

	size_t rfind(basic_string_view str) const
	{
		if (size_ >= str.size_)
		{
			const auto p = std::find_end(begin(), end(), str.begin(), str.end());
			if (p != end())
			{
				return p - begin();
			}

		}
		return npos;
	}

	basic_string_view substr(size_t pos = 0, size_t count = npos) const
	{
		if (pos > size_) throw std::out_of_range("pos > size");
		return basic_string_view(data_ + pos, std::min(count, size_ - pos));
	}

	void remove_prefix(size_t n) { data_ += n; size_ -=n;}
	void remove_suffix(size_t n) { size_ -= n; }

	operator std::basic_string<Char, Traits>() const
	{
		return std::basic_string<Char, Traits>(data_, size_);
	}

	friend bool operator==(basic_string_view lhs, basic_string_view rhs)
	{
		return lhs.size_ == rhs.size_
			&& (lhs.data_ == rhs.data_ || Traits::compare(lhs.data_, rhs.data_, lhs.size_) == 0);
	}

	friend bool operator!=(basic_string_view lhs, basic_string_view rhs)
	{
		return !(lhs == rhs);
	}

	friend std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& os, basic_string_view sv)
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

} // namespace v8pp
#endif

namespace v8pp {
/////////////////////////////////////////////////////////////////////////////
//
// void_t
//
#ifdef __cpp_lib_void_t
using std::void_t;
#else
template<typename...> using void_t = void;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// conjunction, disjunction, negation
//
#ifdef __cpp_lib_logical_traits
using std::conjunction;
using std::disjunction;
using std::negation;
#else
template<bool...>
struct bool_pack {};

template<typename... Bs>
using conjunction = std::is_same<bool_pack<true, Bs::value...>, bool_pack<Bs::value..., true>>;

template<typename... Bs>
using disjunction = std::integral_constant<bool, !conjunction<Bs...>::value>;

template<typename B>
using negation = std::integral_constant<bool, !B::value>;
#endif
} // namespace v8pp

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
// is_string<T>
//
template<typename T>
struct is_string : std::false_type
{
};

template<typename Char, typename Traits, typename Alloc>
struct is_string<std::basic_string<Char, Traits, Alloc>> : std::true_type
{
};

template<typename Char, typename Traits>
struct is_string<v8pp::basic_string_view<Char, Traits>> : std::true_type
{
};

template<>
struct is_string<char const*> : std::true_type
{
};

template<>
struct is_string<char16_t const*> : std::true_type
{
};

template<>
struct is_string<char32_t const*> : std::true_type
{
};

template<>
struct is_string<wchar_t const*> : std::true_type
{
};

/////////////////////////////////////////////////////////////////////////////
//
// is_mapping<T>
//
template<typename T, typename U = void>
struct is_mapping : std::false_type
{
};

template<typename T>
struct is_mapping<T, void_t<typename T::key_type, typename T::mapped_type,
	decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>> : std::true_type
{
};

/////////////////////////////////////////////////////////////////////////////
//
// is_sequence<T>
//
template<typename T, typename U = void>
struct is_sequence : std::false_type
{
};

template<typename T>
struct is_sequence<T, void_t<typename T::value_type,
	decltype(std::declval<T>().begin()),
	decltype(std::declval<T>().end()),
	decltype(std::declval<T>().emplace_back(std::declval<typename T::value_type>()))>> : negation<is_string<T>>
{
};

/////////////////////////////////////////////////////////////////////////////
//
// has_reserve<T>
//
template<typename T, typename U = void>
struct has_reserve : std::false_type
{
	static void reserve(T& container, size_t capacity)
	{
		// no-op
		(void)container;
		(void)capacity;
	}
};

template<typename T>
struct has_reserve<T, void_t<decltype(std::declval<T>().reserve(0))>> : std::true_type
{
	static void reserve(T& container, size_t capacity)
	{
		container.reserve(capacity);
	}
};

/////////////////////////////////////////////////////////////////////////////
//
// is_array<T>
//
template<typename T>
struct is_array : std::false_type
{
	static void check_length(size_t length)
	{
		// no-op for non-arrays
		(void)length;
	}

	template<typename U>
	static void set_element_at(T& container, size_t index, U&& item)
	{
		(void)index;
		container.emplace_back(std::forward<U>(item));
	}
};

template<typename T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type
{
	static void check_length(size_t length)
	{
		if (length != N)
		{
			throw std::runtime_error("Invalid array length: expected "
				+ std::to_string(N) + " actual "
				+ std::to_string(length));
		}
	}

	template<typename U>
	static void set_element_at(std::array<T, N>& array, size_t index, U&& item)
	{
		array[index] = std::forward<U>(item);
	}
};

/////////////////////////////////////////////////////////////////////////////
//
// is_tuple<T>
//
template<typename T>
struct is_tuple : std::false_type
{
};

template<typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type
{
};

/////////////////////////////////////////////////////////////////////////////
//
// is_shared_ptr<T>
//
template<typename T>
struct is_shared_ptr : std::false_type
{
};

template<typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
{
};

/////////////////////////////////////////////////////////////////////////////
//
// Function traits
//
template<typename F>
struct function_traits;

template<typename R, typename... Args>
struct function_traits<R (Args...)>
{
	using return_type = R;
	using arguments = std::tuple<Args...>;
};

// function pointer
template<typename R, typename... Args>
struct function_traits<R (*)(Args...)>
	: function_traits<R (Args...)>
{
	using pointer_type = R (*)(Args...);
};

// member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)>
	: function_traits<R (C&, Args...)>
{
	template<typename D = C>
	using pointer_type = R (D::*)(Args...);
};

// const member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const>
	: function_traits<R (C const&, Args...)>
{
	template<typename D = C>
	using pointer_type = R (D::*)(Args...) const;
};

// volatile member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) volatile>
	: function_traits<R (C volatile&, Args...)>
{
	template<typename D = C>
	using pointer_type = R (D::*)(Args...) volatile;
};

// const volatile member function pointer
template<typename C, typename R, typename... Args>
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
struct function_traits<F&> : function_traits<F>
{
};

template<typename F>
struct function_traits<F&&> : function_traits<F>
{
};

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

	template<typename U, U>
	struct check;

	template<typename>
	static std::true_type test(...);

	template<typename C>
	static std::false_type test(check<void (fallback::*)(), &C::operator()>*);

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
	string_view name() const { return name_; }
	bool operator==(type_info const& other) const { return name_ == other.name_; }
	bool operator!=(type_info const& other) const { return name_ != other.name_; }

private:
	template<typename T>
	friend type_info type_id();

	explicit type_info(string_view name)
		: name_(name)
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
	string_view name = __FUNCSIG__;
	const std::initializer_list<string_view> all_prefixes{ "type_id<", "struct ", "class " };
	const std::initializer_list<string_view> any_suffixes{ ">" };
#elif defined(__clang__) || defined(__GNUC__)
	string_view name = __PRETTY_FUNCTION__;
	const std::initializer_list<string_view> all_prefixes{ "T = " };
	const std::initializer_list<string_view> any_suffixes{ ";", "]" };
#else
#error "Unknown compiler"
#endif
	for (auto&& prefix : all_prefixes)
	{
		const auto p = name.find(prefix);
		if (p != name.npos)
		{
			name.remove_prefix(p + prefix.size());
		}
	}

	for (auto&& suffix : any_suffixes)
	{
		const auto p = name.rfind(suffix);
		if (p != name.npos)
		{
			name.remove_suffix(name.size() - p);
			break;
		}
	}

	return type_info(name);
}

}} // namespace v8pp::detail

#endif // V8PP_UTILITY_HPP_INCLUDED
