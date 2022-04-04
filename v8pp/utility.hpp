#ifndef V8PP_UTILITY_HPP_INCLUDED
#define V8PP_UTILITY_HPP_INCLUDED

#include <functional>
#include <memory>
#include <string_view>
#include <tuple>
#include <optional>
#include <type_traits>

namespace v8pp { namespace detail {

template<typename T>
struct tuple_tail;

template<typename Head, typename... Tail>
struct tuple_tail<std::tuple<Head, Tail...>>
{
	using type = std::tuple<Tail...>;
};

struct none
{
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
struct is_string<std::basic_string_view<Char, Traits>> : std::true_type
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
struct is_mapping_impl : std::false_type
{
};

template<typename T>
struct is_mapping_impl<T, std::void_t<typename T::key_type, typename T::mapped_type,
	decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>> : std::true_type
{
};

template<typename T>
using is_mapping = is_mapping_impl<T>;

/////////////////////////////////////////////////////////////////////////////
//
// is_sequence<T>
//
template<typename T, typename U = void>
struct is_sequence_impl : std::false_type
{
};

template<typename T>
struct is_sequence_impl<T, std::void_t<typename T::value_type,
	decltype(std::declval<T>().begin()), decltype(std::declval<T>().end()),
	decltype(std::declval<T>().emplace_back(std::declval<typename T::value_type>()))>> : std::negation<is_string<T>>
{
};

template<typename T>
using is_sequence = is_sequence_impl<T>;

/////////////////////////////////////////////////////////////////////////////
//
// has_reserve<T>
//
template<typename T, typename U = void>
struct has_reserve_impl : std::false_type
{
};

template<typename T>
struct has_reserve_impl<T, std::void_t<decltype(std::declval<T>().reserve(0))>> : std::true_type
{
};

template<typename T>
using has_reserve = has_reserve_impl<T>;

/////////////////////////////////////////////////////////////////////////////
//
// is_array<T>
//
template<typename T>
struct is_array : std::false_type
{
};

template<typename T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type
{
	static constexpr size_t length = N;
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
// is_optional<T>
//
template<typename T>
struct is_optional : std::false_type
{
};

template<typename T>
struct is_optional<std::optional<T>> : std::true_type
{
};

/////////////////////////////////////////////////////////////////////////////
//
// Function traits
//
template<typename F>
struct function_traits;

template<>
struct function_traits<none>
{
	using return_type = void;
	using arguments = std::tuple<>;
	template<typename D>
	using pointer_type = void;
};

template<typename R, typename... Args>
struct function_traits<R (Args...)>
{
	using return_type = R;
	using arguments = std::tuple<Args...>;
	template<typename D>
	using pointer_type = R (*)(Args...);
};

// function pointer
template<typename R, typename... Args>
struct function_traits<R (*)(Args...)>
	: function_traits<R (Args...)>
{
};

// member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)>
	: function_traits<R (C&, Args...)>
{
	using class_type = C;
	template<typename D>
	using pointer_type = R (D::*)(Args...);
};

// const member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const>
	: function_traits<R (C const&, Args...)>
{
	using class_type = C const;
	template<typename D>
	using pointer_type = R (D::*)(Args...) const;
};

// volatile member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) volatile>
	: function_traits<R (C volatile&, Args...)>
{
	using class_type = C volatile;
	template<typename D>
	using pointer_type = R (D::*)(Args...) volatile;
};

// const volatile member function pointer
template<typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const volatile>
	: function_traits<R (C const volatile&, Args...)>
{
	using class_type = C const volatile;
	template<typename D>
	using pointer_type = R (D::*)(Args...) const volatile;
};

// member object pointer
template<typename C, typename R>
struct function_traits<R (C::*)>
	: function_traits<R (C&)>
{
	using class_type = C;
	template<typename D>
	using pointer_type = R (D::*);
};

// const member object pointer
template<typename C, typename R>
struct function_traits<const R (C::*)>
	: function_traits<R (C const&)>
{
	using class_type = C const;
	template<typename D>
	using pointer_type = const R (D::*);
};

// volatile member object pointer
template<typename C, typename R>
struct function_traits<volatile R (C::*)>
	: function_traits<R (C volatile&)>
{
	using class_type = C volatile;
	template<typename D>
	using pointer_type = volatile R (D::*);
};

// const volatile member object pointer
template<typename C, typename R>
struct function_traits<const volatile R (C::*)>
	: function_traits<R (C const volatile&)>
{
	using class_type = C const volatile;
	template<typename D>
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
	template<typename D>
	using pointer_type = typename callable_traits::template pointer_type<D>;
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
inline constexpr bool is_void_return = std::is_same_v<void, typename function_traits<F>::return_type>;

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
	static constexpr bool value = type::value;
};

template<typename F>
using is_callable = std::integral_constant<bool,
	is_callable_impl<F, std::is_class<F>::value>::value>;

/// Type information for custom RTTI
class type_info
{
public:
	constexpr std::string_view name() const { return name_; }
	constexpr bool operator==(type_info const& other) const { return name_ == other.name_; }
	constexpr bool operator!=(type_info const& other) const { return name_ != other.name_; }

private:
	template<typename T> constexpr friend type_info type_id();

	constexpr explicit type_info(std::string_view name)
		: name_(name)
	{
	}

	std::string_view name_;
};

/// Get type information for type T
/// The idea is borrowed from https://github.com/Manu343726/ctti
template<typename T>
constexpr type_info type_id()
{
#if defined(_MSC_VER) && !defined(__clang__)
	std::string_view name = __FUNCSIG__;
	const std::initializer_list<std::string_view> all_prefixes{ "type_id<", "struct ", "class " };
	const std::initializer_list<std::string_view> any_suffixes{ ">" };
#elif defined(__clang__) || defined(__GNUC__)
	std::string_view name = __PRETTY_FUNCTION__;
	const std::initializer_list<std::string_view> all_prefixes{ "T = " };
	const std::initializer_list<std::string_view> any_suffixes{ ";", "]" };
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
