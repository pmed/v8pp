#pragma once

#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"

namespace v8pp::detail {

template<typename F, typename T, typename U = typename call_from_v8_traits<F>::template arg_type<0>>
inline constexpr bool function_with_object = std::is_member_function_pointer_v<F> ||
	(std::is_lvalue_reference_v<U> && std::is_base_of_v<T, std::remove_cv_t<std::remove_reference_t<U>>>);

template<typename F, size_t Offset, typename CallTraits = call_from_v8_traits<F>>
inline constexpr bool is_getter = CallTraits::arg_count == 0 + Offset
	&& !std::same_as<typename function_traits<F>::return_type, void>;

template<typename F, size_t Offset, typename CallTraits = call_from_v8_traits<F>>
inline constexpr bool is_direct_getter = CallTraits::arg_count == 2 + Offset
	&& std::is_convertible_v<typename CallTraits::template arg_type<0 + Offset>, v8::Local<v8::Name>>
	&& std::same_as<typename CallTraits::template arg_type<1 + Offset>, v8::PropertyCallbackInfo<v8::Value> const&>
	&& std::same_as<typename function_traits<F>::return_type, void>;

template<typename F, size_t Offset, typename CallTraits = call_from_v8_traits<F>>
inline constexpr bool is_isolate_getter = CallTraits::arg_count == 1 + Offset
	&& is_first_arg_isolate<F, Offset>
	&& !std::same_as<typename function_traits<F>::return_type, void>;

template<typename F, size_t Offset, typename CallTraits = call_from_v8_traits<F>>
inline constexpr bool is_setter = CallTraits::arg_count == 1 + Offset;

template<typename F, size_t Offset, typename CallTraits = call_from_v8_traits<F>>
inline constexpr bool is_direct_setter = CallTraits::arg_count == 3 + Offset
	&& std::is_convertible_v<typename CallTraits::template arg_type<0 + Offset>, v8::Local<v8::Name>>
	&& std::same_as<typename CallTraits::template arg_type<1 + Offset>, v8::Local<v8::Value>>
	&& std::same_as<typename CallTraits::template arg_type<2 + Offset>, v8::PropertyCallbackInfo<void> const&>
	&& std::same_as<typename function_traits<F>::return_type, void>;

template<typename F, size_t Offset, typename CallTraits = call_from_v8_traits<F>>
inline constexpr bool is_isolate_setter = CallTraits::arg_count == 2 + Offset
	&& is_first_arg_isolate<F, Offset>;

template<typename Get, typename... ObjArg>
void property_get(Get& getter, v8::Local<v8::Name> name,
	v8::PropertyCallbackInfo<v8::Value> const& info, ObjArg&... obj)
{
	constexpr size_t offset = sizeof...(ObjArg) == 0 || std::is_member_function_pointer_v<Get> ? 0 : 1;

	v8::Isolate* isolate = info.GetIsolate();

	if constexpr (is_direct_getter<Get, offset>)
	{
		(void)isolate;
		std::invoke(getter, obj..., name, info);
	}
	else if constexpr (is_isolate_getter<Get, offset>)
	{
		(void)name;
		info.GetReturnValue().Set(to_v8(isolate, std::invoke(getter, obj..., isolate)));
	}
	else if constexpr (is_getter<Get, offset>)
	{
		(void)name;
		info.GetReturnValue().Set(to_v8(isolate, std::invoke(getter, obj...)));
	}
	else
	{
		(void)getter;
		(void)name;
		(void)info;
		(void)isolate;
		//static_assert(false, "Unsupported getter type");
	}
}

template<typename Set, typename... ObjArg>
void property_set(Set& setter, v8::Local<v8::Name> name, v8::Local<v8::Value> value,
	v8::PropertyCallbackInfo<void> const& info, ObjArg&... obj)
{
	constexpr size_t offset = sizeof...(ObjArg) == 0 || std::is_member_function_pointer_v<Set> ? 0 : 1;

	v8::Isolate* isolate = info.GetIsolate();

	if constexpr (is_direct_setter<Set, offset>)
	{
		(void)isolate;
		std::invoke(setter, obj..., name, value, info);
	}
	else if constexpr (is_isolate_setter<Set, offset>)
	{
		(void)name;
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1 + offset>;
		std::invoke(setter, obj..., isolate, v8pp::from_v8<value_type>(isolate, value));
	}
	else if constexpr (is_setter<Set, offset>)
	{
		(void)name;
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0 + offset>;
		std::invoke(setter, obj..., v8pp::from_v8<value_type>(isolate, value));
	}
	else
	{
		(void)setter;
		(void)name;
		(void)value;
		(void)info;
		(void)isolate;
		//static_assert(false, "Unsupported setter type");
	}
}

template<typename Property, typename Traits, typename GetClass>
void property_get(v8::Local<v8::Name> name, v8::PropertyCallbackInfo<v8::Value> const& info)
try
{
	auto&& property = detail::external_data::get<Property>(info.Data());

	if constexpr (std::same_as<GetClass, none>)
	{
		property_get(property.getter, name, info);
	}
	else
	{
		auto obj = v8pp::class_<GetClass, Traits>::unwrap_object(info.GetIsolate(), info.This());
		property_get(property.getter, name, info, *obj);
	}
}
catch (std::exception const& ex)
{
	if (info.ShouldThrowOnError())
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}
}

template<typename Property, typename Traits, typename Set, typename SetClass>
void property_set(v8::Local<v8::Name> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
try
{
	auto&& property = detail::external_data::get<Property>(info.Data());

	if constexpr (std::same_as<SetClass, none>)
	{
		property_set(property.setter, name, value, info);
	}
	else
	{
		auto obj = v8pp::class_<SetClass, Traits>::unwrap_object(info.GetIsolate(), info.This());
		property_set(property.setter, name, value, info, *obj);
	}
}
catch (std::exception const& ex)
{
	if (info.ShouldThrowOnError())
	{
		info.GetIsolate()->ThrowException(throw_ex(info.GetIsolate(), ex.what()));
	}
	// TODO: info.GetReturnValue().Set(false);
}

} // namespace v8pp::detail

namespace v8pp {

/// Property with get and set functions
template<typename Get, typename Set, typename GetClass, typename SetClass>
struct property final
{
	Get getter;
	Set setter;

	static constexpr bool is_readonly = false;

	property() = default;
	property(Get&& getter, Set&& setter)
		: getter(std::move(getter))
		, setter(std::move(setter))
	{
	}

	template<typename Traits>
	static void get(v8::Local<v8::Name> name, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		detail::property_get<property, Traits, GetClass>(name, info);
	}

	template<typename Traits>
	static void set(v8::Local<v8::Name> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		detail::property_set<property, Traits, Set, SetClass>(name, value, info);
	}
};

/// Read-only property class specialization for get only method
template<typename Get, typename GetClass>
struct property<Get, detail::none, GetClass, detail::none> final
{
	Get getter;

	static constexpr bool is_readonly = true;

	property() = default;
	property(Get&& getter, detail::none)
		: getter(std::move(getter))
	{
	}

	template<typename Traits>
	static void get(v8::Local<v8::Name> name, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		detail::property_get<property, Traits, GetClass>(name, info);
	}

	template<typename Traits>
	static void set(v8::Local<v8::Name> name, v8::Local<v8::Value>, v8::PropertyCallbackInfo<void> const& info)
	{
		//assert(false && "read-only property");
		if (info.ShouldThrowOnError())
		{
			info.GetIsolate()->ThrowException(v8pp::to_v8(info.GetIsolate(),
				"read-only property " + from_v8<std::string>(info.GetIsolate(), name)));
		}
		// TODO: info.GetReturnValue().Set(false);
	}
};

} // namespace v8pp
