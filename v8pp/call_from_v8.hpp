#ifndef V8PP_CALL_FROM_V8_HPP_INCLUDED
#define V8PP_CALL_FROM_V8_HPP_INCLUDED

#include <functional>
#include <utility>

#include <v8.h>

#include "v8pp/convert.hpp"
#include "v8pp/utility.hpp"

namespace v8pp { namespace detail {

template<typename>
struct optional_count_impl;

template<typename... Ts>
struct optional_count_impl<std::tuple<Ts...>>
{
	constexpr static std::size_t count =
		(0 + ... + (is_optional<Ts>::value ? 1 : 0));
};

template<typename Tuple>
constexpr auto optional_count() -> std::size_t
{
	return optional_count_impl<Tuple>::count;
}

template<typename F, size_t Offset = 0>
struct call_from_v8_traits
{
	static constexpr size_t offset = Offset;
	static constexpr bool is_mem_fun = std::is_member_function_pointer<F>::value;
	using arguments = typename function_traits<F>::arguments;
	static constexpr size_t optional_arg_count = optional_count<arguments>();

	static constexpr size_t arg_count =
		std::tuple_size<arguments>::value - is_mem_fun - offset;

	template<size_t Index, bool>
	struct tuple_element
	{
		using type = typename std::tuple_element<Index, arguments>::type;
	};

	template<size_t Index>
	struct tuple_element<Index, false>
	{
		using type = void;
	};

	template<size_t Index>
	using arg_type = typename tuple_element < Index + is_mem_fun, Index<(arg_count + offset)>::type;

	template<typename Arg, typename Traits,
		typename T = std::remove_reference_t<Arg>,
		typename U = std::remove_pointer_t<T>>
	using arg_converter = typename std::conditional_t<
		is_wrapped_class<std::remove_cv_t<U>>::value,
		std::conditional_t<std::is_pointer_v<T>,
			typename Traits::template convert_ptr<U>,
			typename Traits::template convert_ref<U>>,
		convert<std::remove_cv_t<T>>>;

	template<size_t Index, typename Traits>
	static decltype(auto) arg_from_v8(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		// might be reference
		return (arg_converter<arg_type<Index>, Traits>::from_v8(args.GetIsolate(), args[Index - offset]));
	}
};

template<typename F, size_t Offset, typename CallTraits = call_from_v8_traits<F>>
inline constexpr bool is_direct_args = CallTraits::arg_count == (Offset + 1) &&
	std::is_same_v<typename CallTraits::template arg_type<Offset>, v8::FunctionCallbackInfo<v8::Value> const&>;

template<typename F, size_t Offset = 0, typename CallTraits = call_from_v8_traits<F>>
inline constexpr bool is_first_arg_isolate = CallTraits::arg_count != (Offset + 0) &&
	std::is_same_v<typename CallTraits::template arg_type<Offset>, v8::Isolate*>;

template<typename Traits, typename F, typename CallTraits, size_t... Indices, typename... ObjArg>
decltype(auto) call_from_v8_impl(F&& func, v8::FunctionCallbackInfo<v8::Value> const& args,
	CallTraits, std::index_sequence<Indices...>, ObjArg&&... obj)
{
	(void)args;
	return (std::invoke(func, std::forward<ObjArg>(obj)...,
		CallTraits::template arg_from_v8<Indices + CallTraits::offset, Traits>(args)...));
}

template<typename Traits, typename F, typename... ObjArg>
decltype(auto) call_from_v8(F&& func, v8::FunctionCallbackInfo<v8::Value> const& args, ObjArg&... obj)
{
	constexpr bool with_isolate = is_first_arg_isolate<F>;
	if constexpr (is_direct_args<F, with_isolate>)
	{
		if constexpr (with_isolate)
		{
			return (std::invoke(func, obj..., args.GetIsolate(), args));
		}
		else
		{
			return (std::invoke(func, obj..., args));
		}
	}
	else
	{
		using call_traits = call_from_v8_traits<F, with_isolate>;
		using indices = std::make_index_sequence<call_traits::arg_count>;

		if (args.Length() > call_traits::arg_count || args.Length() < call_traits::arg_count - call_traits::optional_arg_count)
		{
			throw std::runtime_error("argument count does not match function definition");
		}

		if constexpr (with_isolate)
		{
			return (call_from_v8_impl<Traits>(std::forward<F>(func), args,
				call_traits{}, indices{}, obj..., args.GetIsolate()));
		}
		else
		{
			return (call_from_v8_impl<Traits>(std::forward<F>(func), args,
				call_traits{}, indices{}, obj...));
		}
	}
}

}} // namespace v8pp::detail

#endif // V8PP_CALL_FROM_V8_HPP_INCLUDED
