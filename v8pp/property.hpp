//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_PROPERTY_HPP_INCLUDED
#define V8PP_PROPERTY_HPP_INCLUDED

#include <cassert>

#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"

namespace v8pp {

template<typename Get, typename Set, typename GetClass, typename SetClass>
struct property;

namespace detail {

struct getter_tag {};
struct direct_getter_tag {};
struct isolate_getter_tag {};

struct setter_tag {};
struct direct_setter_tag {};
struct isolate_setter_tag {};

template<typename F, typename T, typename U = typename call_from_v8_traits<F>::template arg_type<0>>
using function_with_object = std::integral_constant<bool,
	std::is_member_function_pointer<F>::value ||
	(std::is_lvalue_reference<U>::value &&
	std::is_base_of<T, typename std::remove_cv<typename std::remove_reference<U>::type>::type>::value)
>;

template<typename F, size_t Offset>
using is_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 0 + Offset &&
	!is_void_return<F>::value
>;

template<typename F, size_t Offset>
using is_direct_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 2 + Offset &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0 + Offset>,
		v8::Local<v8::String>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1 + Offset>,
		v8::PropertyCallbackInfo<v8::Value> const&>::value &&
	is_void_return<F>::value
>;

template<typename F, size_t Offset>
using is_isolate_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 1 + Offset &&
	is_first_arg_isolate<F, Offset>::value &&
	!is_void_return<F>::value
>;

template<typename F, size_t Offset>
using is_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 1 + Offset
>;

template<typename F, size_t Offset>
using is_direct_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 3 + Offset &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0 + Offset>,
		v8::Local<v8::String>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1 + Offset>,
		v8::Local<v8::Value>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<2 + Offset>,
		v8::PropertyCallbackInfo<void> const&>::value &&
	is_void_return<F>::value
>;

template<typename F, size_t Offset>
using is_isolate_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 2 + Offset &&
	is_first_arg_isolate<F, Offset>::value
>;

template<typename F, size_t Offset>
using select_getter_tag = typename std::conditional<is_direct_getter<F, Offset>::value,
	direct_getter_tag,
	typename std::conditional<is_isolate_getter<F, Offset>::value,
		isolate_getter_tag, getter_tag>::type
>::type;

template<typename F, size_t Offset>
using select_setter_tag = typename std::conditional<is_direct_setter<F, Offset>::value,
	direct_setter_tag,
	typename std::conditional<is_isolate_setter<F, Offset>::value,
		isolate_setter_tag, setter_tag>::type
>::type;

template<typename Get, typename GetClass>
struct r_property_impl
{
	using class_type = GetClass;

	Get getter;

	r_property_impl() = default;
	r_property_impl(Get&& getter)
		: getter(std::move(getter))
	{
	}

	void get_impl(class_type& obj, v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info,
		getter_tag)
	{
		info.GetReturnValue().Set(to_v8(info.GetIsolate(), std::invoke(getter, obj)));
	}

	void get_impl(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info,
		getter_tag)
	{
		info.GetReturnValue().Set(to_v8(info.GetIsolate(), std::invoke(getter)));
	}

	void get_impl(class_type& obj, v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info,
		direct_getter_tag)
	{
		std::invoke(getter, obj, name, info);
	}

	void get_impl(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info, direct_getter_tag)
	{
		std::invoke(getter, name, info);
	}

	void get_impl(class_type& obj, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info,
		isolate_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, std::invoke(getter, obj, isolate)));
	}

	void get_impl(v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, isolate_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, std::invoke(getter, isolate)));
	}

	template<typename Property, typename Traits>
	static void get(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	try
	{
		if constexpr (std::is_same_v<class_type, none>)
		{
			detail::external_data::get<Property>(info.Data()).
				get_impl(name, info, select_getter_tag<Get, 0>());
		}
		else
		{
			auto obj = v8pp::class_<class_type, Traits>::unwrap_object(info.GetIsolate(), info.This());

			using is_mem_fun = std::is_member_function_pointer<Get>;
			using offset = std::integral_constant<size_t, is_mem_fun::value ? 0 : 1>;
			detail::external_data::get<Property>(info.Data())
				.get_impl(*obj, name, info, select_getter_tag<Get, offset::value>());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}

	template<typename Property, typename Traits>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value>,
		v8::PropertyCallbackInfo<void> const& info)
	{
		assert(false && "read-only property");
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(),
			"read-only property " + from_v8<std::string>(info.GetIsolate(), name)));
	}
};

template<typename Get, typename Set, typename GetClass, typename SetClass>
struct rw_property_impl
	: r_property_impl<Get, GetClass>
{
	using class_type = SetClass;

	Set setter;

	rw_property_impl() = default;
	rw_property_impl(Get&& getter, Set&& setter)
		: r_property_impl<Get, GetClass>(std::move(getter))
		, setter(std::move(setter))
	{
	}

	void set_impl(class_type& obj, v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info,
		setter_tag)
	{
		using is_mem_fun = std::is_member_function_pointer<Set>;
		using offset = std::integral_constant<size_t, is_mem_fun::value ? 0 : 1>;
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0 + offset()>;
		std::invoke(setter, obj, v8pp::from_v8<value_type>(info.GetIsolate(), value));
	}

	void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info,
		setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;
		std::invoke(setter, v8pp::from_v8<value_type>(info.GetIsolate(), value));
	}

	void set_impl(class_type& obj, v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info,
		direct_setter_tag)
	{
		std::invoke(setter, obj, name, value, info);
	}

	void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info,
		direct_setter_tag)
	{
		std::invoke(setter, name, value, info);
	}

	void set_impl(class_type& obj, v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info,
		isolate_setter_tag)
	{
		using is_mem_fun = std::is_member_function_pointer<Set>;
		using offset = std::integral_constant<size_t, is_mem_fun::value ? 0 : 1>;
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1 + offset()>;

		v8::Isolate* isolate = info.GetIsolate();

		std::invoke(setter, obj, isolate, v8pp::from_v8<value_type>(isolate, value));
	}

	void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info,
		isolate_setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();

		std::invoke(setter, isolate, v8pp::from_v8<value_type>(isolate, value));
	}

	template<typename Property, typename Traits>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info)
	try
	{
		if constexpr (std::is_same_v<class_type, none>)
		{
			detail::external_data::get<Property>(info.Data()).
				set_impl(name, value, info, select_setter_tag<Set, 0>());
		}
		else
		{
			auto obj = v8pp::class_<class_type, Traits>::unwrap_object(info.GetIsolate(), info.This());

			using is_mem_fun = std::is_member_function_pointer<Set>;
			using offset = std::integral_constant<size_t, is_mem_fun::value ? 0 : 1>;
			detail::external_data::get<Property>(info.Data()).
				set_impl(*obj, name, value, info, select_setter_tag<Set, offset::value>());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}
};

} // namespace detail

/// Property with get and set functions
template<typename Get, typename Set, typename GetClass, typename SetClass>
struct property
	: detail::rw_property_impl<Get, Set, GetClass, SetClass>
{
	static constexpr bool is_readonly = false;

	property() = default;
	property(Get&& getter, Set&& setter)
		: detail::rw_property_impl<Get, Set, GetClass, SetClass>(std::move(getter), std::move(setter))
	{
	}
};

/// Read-only property class specialization for get only method
template<typename Get, typename GetClass>
struct property<Get, detail::none, GetClass, detail::none>
	: detail::r_property_impl<Get, GetClass>
{
	static constexpr bool is_readonly = true;

	property() = default;
	property(Get&& getter, detail::none)
		: detail::r_property_impl<Get, GetClass>(std::move(getter))
	{
	}
};

} // namespace v8pp

#endif // V8PP_PROPERTY_HPP_INCLUDED
