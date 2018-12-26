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

template<
	typename Get, 
	typename Set, 
	bool GetIsClassBased = std::is_member_function_pointer<Get>::value, 
	bool SetIsClassBased = std::is_member_function_pointer<Set>::value
>
struct property;

namespace detail {

struct getter_tag {};
struct direct_getter_tag {};
struct isolate_getter_tag {};

struct setter_tag {};
struct direct_setter_tag {};
struct isolate_setter_tag {};

template<typename F, size_t Offset=0>
using is_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == Offset && !is_void_return<F>::value>;

template<typename F, size_t Offset=0>
using is_direct_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == (2 + Offset) &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0 + Offset>,
		v8::Local<v8::String>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1 + Offset>,
		v8::PropertyCallbackInfo<v8::Value> const&>::value &&
	is_void_return<F>::value
>;

template<typename F, size_t Offset=0>
using is_isolate_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == (1 + Offset) &&
	is_first_arg_isolate<F, Offset>::value &&
	!is_void_return<F>::value>;

template<typename F, size_t Offset=0>
using is_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == (1 + Offset) && is_void_return<F>::value>;

template<typename F, size_t Offset=0>
using is_direct_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == (3 + Offset) &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0 + Offset>,
		v8::Local<v8::String>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1 + Offset>,
		v8::Local<v8::Value>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<2 + Offset>,
		v8::PropertyCallbackInfo<void> const&>::value &&
	is_void_return<F>::value
>;

template<typename F, size_t Offset=0>
using is_isolate_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == (2 + Offset) &&
	is_first_arg_isolate<F, Offset>::value &&
	is_void_return<F>::value>;

template<typename T, typename F>
using is_class_based_setter = 
	typename std::conditional<
	std::is_same<F, detail::none>::value,
	std::false_type,
	typename std::conditional<
		std::is_member_function_pointer<F>::value,
		std::true_type,
		std::integral_constant<
			bool,
			std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, T&>::value &&
			is_void_return<F>::value
		>
	>::type
>::type;

template<typename T, typename F>
using is_class_based_getter = typename std::conditional<
	std::is_member_function_pointer<F>::value,
	std::true_type,
	std::integral_constant<
		bool,
		std::is_same<typename call_from_v8_traits<F>::template arg_type<0>, T&>::value
	>
>::type;

template<typename F, size_t Offset=0>
using select_getter_tag = typename std::conditional<is_direct_getter<F, Offset>::value,
	direct_getter_tag,
	typename std::conditional<is_isolate_getter<F, Offset>::value,
		isolate_getter_tag, getter_tag>::type
>::type;

template<typename F, size_t Offset=0>
using select_setter_tag = typename std::conditional<is_direct_setter<F, Offset>::value,
	direct_setter_tag,
	typename std::conditional<is_isolate_setter<F, Offset>::value,
		isolate_setter_tag, setter_tag>::type
>::type;

template<typename Get, typename Set, bool GetIsClassBased, bool SetIsClassBased>
struct r_property_impl;

template<typename Get, typename Set, bool GetIsClassBased, bool SetIsClassBased>
struct rw_property_impl;

template<typename Get, typename Set, bool SetIsClassBased>
struct r_property_impl<Get, Set, true, SetIsClassBased>
{
	using property_type = property<Get, Set, true, SetIsClassBased>;

	using is_member = std::is_member_function_pointer<Get>;

	using offset = typename std::conditional<
		std::is_member_function_pointer<Get>::value,
		std::integral_constant<int,0>, 
		std::integral_constant<int,1>
	>::type;

	using class_type = typename std::decay<typename std::tuple_element<0,
		typename function_traits<Get>::arguments>::type>::type;

	static_assert(is_getter<Get, offset::value>::value
		|| is_direct_getter<Get, offset::value>::value
		|| is_isolate_getter<Get, offset::value>::value,
		"property get function must be either `T ()` or \
		`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or \
		`T (v8::Isolate*)`");

	static void get_impl(class_type& obj, Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, getter_tag, std::true_type)
	{
		info.GetReturnValue().Set(to_v8(info.GetIsolate(), (obj.*get)()));
	}

	static void get_impl(
		class_type& obj, 
		Get get, 
		v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, 
		getter_tag, 
		std::false_type
	)
	{
		info.GetReturnValue().Set(to_v8(info.GetIsolate(), (*get)(obj)));
	}

	static void get_impl(class_type& obj, Get get,
		v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		direct_getter_tag, std::true_type)
	{
		(obj.*get)(name, info);
	}

	static void get_impl(
		class_type& obj, 
		Get get,
		v8::Local<v8::String> name, 
		v8::PropertyCallbackInfo<v8::Value> const& info,
		direct_getter_tag, 
		std::false_type
	)
	{
		(*get)(obj, name, info);
	}

	static void get_impl(class_type& obj, Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, isolate_getter_tag, std::true_type)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (obj.*get)(isolate)));
	}

	static void get_impl(
		class_type& obj, 
		Get get, 
		v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, 
		isolate_getter_tag, 
		std::false_type
	)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (*get)(obj, isolate)));
	}

	template<typename Traits>
	static void get(v8::Local<v8::String> name,
		const v8::PropertyCallbackInfo<v8::Value>& info)
	try
	{
		auto obj = v8pp::class_<class_type, Traits>::unwrap_object(info.GetIsolate(), info.This());
		assert(obj);

		const property_type& prop = detail::get_external_data<property_type>(info.Data());
		assert(prop.getter);

		if (obj && prop.getter)
		{
			get_impl(*obj, prop.getter, name, info, select_getter_tag<Get, offset::value>(), is_member());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}

	template<typename Traits>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		const v8::PropertyCallbackInfo<void>& info)
	{
		//set(name, value, info);
		assert(false && "read-only property");
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(),
			"read-only property " + from_v8<std::string>(info.GetIsolate(), name)));
	}

};

template<typename Get, typename Set, bool SetIsClassBased>
struct r_property_impl<Get, Set, false, SetIsClassBased>
{
	using property_type = property<Get, Set, false>;

	static void get_impl(Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, getter_tag)
	{
		info.GetReturnValue().Set(to_v8(info.GetIsolate(), get()));
	}

	static void get_impl(Get get, v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info, direct_getter_tag)
	{
		get(name, info);
	}

	static void get_impl(Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, isolate_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (get)(isolate)));
	}

	template<typename Traits>
	static void get(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		get(name, info);
	}

	static void get(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	try
	{
		property_type const& prop = detail::get_external_data<property_type>(info.Data());
		assert(prop.getter);

		if (prop.getter)
		{
			get_impl(prop.getter, name, info, select_getter_tag<Get>());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}

	template<typename Traits>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		const v8::PropertyCallbackInfo<void>& info)
	{
		assert(false && "read-only property");
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(),
			"read-only property " + from_v8<std::string>(info.GetIsolate(), name)));
	}

};

template<typename Get, typename Set, bool GetIsClassBased>
struct rw_property_impl<Get, Set, GetIsClassBased, true>
	: r_property_impl<Get, Set, GetIsClassBased, true>
{
	using property_type = property<Get, Set, GetIsClassBased, true>;

	using class_type = typename std::decay<typename std::tuple_element<0,
		typename function_traits<Set>::arguments>::type>::type;

	using is_member = std::is_member_function_pointer<Set>;

	using offset = typename std::conditional<
		std::is_member_function_pointer<Get>::value,
		std::integral_constant<int,0>, 
		std::integral_constant<int,1>
	>::type;

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String>,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		setter_tag, std::true_type)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

		(obj.*set)(v8pp::from_v8<value_type>(info.GetIsolate(), value));
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String> name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		setter_tag, std::false_type)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		(*set)(obj, v8pp::from_v8<value_type>(info.GetIsolate(), value));
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String> name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		direct_setter_tag, std::true_type)
	{
		(obj.*set)(name, value, info);
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String> name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		direct_setter_tag, std::false_type)
	{
		(*set)(obj, name, value, info);
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String>, 
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		isolate_setter_tag, std::true_type)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();

		(obj.*set)(isolate, v8pp::from_v8<value_type>(isolate, value));
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String>, 
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		isolate_setter_tag, std::false_type)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<2>;

		v8::Isolate* isolate = info.GetIsolate();

		(*set)(obj, isolate, v8pp::from_v8<value_type>(isolate, value));
	}

	template<typename Traits>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info)
	try
	{
		auto obj = v8pp::class_<class_type, Traits>::unwrap_object(info.GetIsolate(), info.This());
		assert(obj);

		property_type const& prop = detail::get_external_data<property_type>(info.Data());
		assert(prop.setter);

		if (obj && prop.setter)
		{
			set_impl(*obj, prop.setter, name, value, info, select_setter_tag<Set, offset::value>(), is_member());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}
};

template<typename Get, typename Set, bool GetIsClassBased>
struct rw_property_impl<Get, Set, GetIsClassBased, false>
	: r_property_impl<Get, Set, GetIsClassBased, false>
{
	using property_type = property<Get, Set, GetIsClassBased, false>;

	static void set_impl(Set set, v8::Local<v8::String>,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

		set(v8pp::from_v8<value_type>(info.GetIsolate(), value));
	}

	static void set_impl(Set set, v8::Local<v8::String> name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		direct_setter_tag)
	{
		set(name, value, info);
	}

	static void set_impl(Set set, v8::Local<v8::String>,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		isolate_setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();

		set(isolate, v8pp::from_v8<value_type>(info.GetIsolate(), value));
	}

	template<typename Traits>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		const v8::PropertyCallbackInfo<void>& info)
	{
		set(name, value, info);
	}

	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		const v8::PropertyCallbackInfo<void>& info)
	try
	{
		property_type const& prop = detail::get_external_data<property_type>(info.Data());
		assert(prop.setter);

		if (prop.setter)
		{
			set_impl(prop.setter, name, value, info, select_setter_tag<Set>());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}
};

} // namespace detail

/// Property with get and set functions
template<
	typename Get, 
	typename Set,
	bool GetIsClassBased, 
	bool SetIsClassBased
>
struct property
	: detail::rw_property_impl<Get, Set, GetIsClassBased, SetIsClassBased>
{
	using get_offset = typename std::conditional<
		!std::is_member_function_pointer<Get>::value && GetIsClassBased,
		std::integral_constant<size_t, 1>,
		std::integral_constant<size_t, 0>
	>::type;

	using set_offset = typename std::conditional<
		!std::is_member_function_pointer<Set>::value && SetIsClassBased,
		std::integral_constant<size_t, 1>,
		std::integral_constant<size_t, 0>
	>::type;

	static_assert(detail::is_getter<Get, get_offset::value>::value
		|| detail::is_direct_getter<Get, get_offset::value>::value
		|| detail::is_isolate_getter<Get, get_offset::value>::value,
		"property get function must be either `T ()` or "
		"`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
		"`T (v8::Isolate*)`");

	static_assert(detail::is_setter<Set, set_offset::value>::value
		|| detail::is_direct_setter<Set, set_offset::value>::value
		|| detail::is_isolate_setter<Set, set_offset::value>::value
		|| std::is_same<Set, detail::none>::value,
		"property set function must be either `void (T)` or \
		`void (v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)` or \
		`void (v8::Isolate*, T)`");

	Get getter;
	Set setter;

	enum { is_readonly = false };

	property(Get getter, Set setter)
		: getter(getter)
		, setter(setter)
	{
	}

	template<typename OtherGet, typename OtherSet>
	property(property<OtherGet, OtherSet> const& other)
		: getter(other.getter)
		, setter(other.setter)
	{
	}
};

/// Read-only property class specialization for get only method
template<
	typename Get,
	bool GetIsClassBased
>
struct property<Get, detail::none, GetIsClassBased, false>
	: detail::r_property_impl<Get, detail::none, GetIsClassBased, false>
{
	using get_offset = typename std::conditional<
		!std::is_member_function_pointer<Get>::value && GetIsClassBased,
		std::integral_constant<size_t, 1>,
		std::integral_constant<size_t, 0>
	>::type;

	static_assert(detail::is_getter<Get, get_offset::value>::value
		|| detail::is_direct_getter<Get, get_offset::value>::value
		|| detail::is_isolate_getter<Get, get_offset::value>::value,
		"property get function must be either `T ()` or "
		"void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
		"`T (v8::Isolate*)`");

	Get getter;

	enum { is_readonly = true };

	property(Get getter, detail::none)
		: getter(getter)
	{
	}

	template<typename OtherGet>
	explicit property(property<OtherGet, OtherGet> const& other)
		: getter(other.getter)
	{
	}
};

} // namespace v8pp

#endif // V8PP_PROPERTY_HPP_INCLUDED
