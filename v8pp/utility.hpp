#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace v8pp::detail {

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

template<typename Char>
concept WideChar = std::same_as<Char, char16_t> || std::same_as<Char, char32_t> || std::same_as<Char, wchar_t>;

template<typename T>
concept String = std::same_as<T, std::string> || std::same_as<T, std::string_view>
	|| std::same_as<T, std::wstring> || std::same_as<T, std::wstring_view>
	|| std::same_as<T, std::u16string> || std::same_as<T, std::u16string_view>
	|| std::same_as<T, std::u32string> || std::same_as<T, std::u32string_view>
	|| std::same_as<T, char const*> || std::same_as<T, char16_t const*>
	||	std::same_as<T, char32_t const*> || std::same_as<T, wchar_t const*>;

template<typename T>
concept Mapping = requires(T t) {
	typename T::key_type;
	typename T::mapped_type;
	t.begin();
	t.end();
};


template<typename T>
concept Sequence = requires(T t) {
	typename T::value_type;
	t.begin();
	t.end();
	t.emplace_back(std::declval<typename T::value_type>());
} && !String<T>;

template<typename T>
concept HasReserve = requires(T t) {
	t.reserve(0);
};

template<typename T>
concept Array = requires(T t) {
	typename std::tuple_size<T>::type;
	typename T::value_type;
	t.begin();
	t.end();
};

template<typename T>
concept Tuple = requires(T t) {
	typename std::tuple_size<T>::type;
	typename std::tuple_element<0, T>::type;
	typename std::tuple_element_t<0, T>;
	std::get<0>(t);
} && !Array<T>;

template<typename T>
concept SharedPtr = std::same_as<T, std::shared_ptr<typename T::element_type>>;

template<typename T>
using is_string = std::bool_constant<String<T>>;

template<typename T>
using is_mapping = std::bool_constant<Mapping<T>>;

template<typename T>
using is_sequence = std::bool_constant<Sequence<T>>;

template<typename T>
using has_reserve = std::bool_constant<HasReserve<T>>;

template<typename T>
using is_array = std::bool_constant<Array<T>>;

template<typename T>
using is_tuple = std::bool_constant<Tuple<T>>;

template<typename T>
using is_shared_ptr = std::bool_constant<SharedPtr<T>>;

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

} // namespace v8pp::detail
