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

template<typename Get, typename Set>
struct property_;

namespace detail {

struct getter_tag {};
struct direct_getter_tag {};
struct isolate_getter_tag {};

struct setter_tag {};
struct direct_setter_tag {};
struct isolate_setter_tag {};

template<typename F>
using is_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 0 && !is_void_return<F>::value>;

template<typename F>
using is_direct_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 2 &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>,
		v8::Local<v8::String>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1>,
		v8::PropertyCallbackInfo<v8::Value> const&>::value &&
	is_void_return<F>::value
>;

template<typename F>
using is_isolate_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 1 &&
	is_first_arg_isolate<F>::value &&
	!is_void_return<F>::value>;

template<typename F>
using is_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 1 && is_void_return<F>::value>;

template<typename F>
using is_direct_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 3 &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<0>,
		v8::Local<v8::String>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<1>,
		v8::Local<v8::Value>>::value &&
	std::is_same<typename call_from_v8_traits<F>::template arg_type<2>,
		v8::PropertyCallbackInfo<void> const&>::value &&
	is_void_return<F>::value
>;

template<typename F>
using is_isolate_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 2 &&
	is_first_arg_isolate<F>::value &&
	is_void_return<F>::value>;

template<typename F>
using select_getter_tag = typename std::conditional<is_direct_getter<F>::value,
	direct_getter_tag,
	typename std::conditional<is_isolate_getter<F>::value,
		isolate_getter_tag, getter_tag>::type
>::type;

template<typename F>
using select_setter_tag = typename std::conditional<is_direct_setter<F>::value,
	direct_setter_tag,
	typename std::conditional<is_isolate_setter<F>::value,
		isolate_setter_tag, setter_tag>::type
>::type;

template<typename Get, typename Set, bool get_is_mem_fun>
struct r_property_impl;

template<typename Get, typename Set, bool set_is_mem_fun>
struct rw_property_impl;

template<typename Get, typename Set>
struct r_property_impl<Get, Set, true>
{
	using Property = property_<Get, Set>;

	using class_type = typename std::tuple_element<0,
		typename function_traits<Get>::arguments> ::type;

	static_assert(is_getter<Get>::value
		|| is_direct_getter<Get>::value
		|| is_isolate_getter<Get>::value,
		"property get function must be either `T ()` or \
					`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or \
					`T (v8::Isolate*)`");

	static void get_impl(class_type& obj, Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (obj.*get)()));
	}

	static void get_impl(class_type& obj, Get get,
		v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		direct_getter_tag)
	{
		(obj.*get)(name, info);
	}

	static void get_impl(class_type& obj, Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, isolate_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, (obj.*get)(isolate)));
	}

	static void get(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		class_type& obj = v8pp::from_v8<class_type&>(isolate, info.This());

		Property prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			get_impl(obj, prop.get_, name, info, select_getter_tag<Get>());
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	static void set(v8::Local<v8::String>, v8::Local<v8::Value>, v8::PropertyCallbackInfo<void> const&)
	{
		assert(false && "never should be called");
	}
};

template<typename Get, typename Set>
struct r_property_impl<Get, Set, false>
{
	using Property = property_<Get, Set>;

	static void get_impl(Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, get()));
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

	static void get(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			get_impl(prop.get_, name, info, select_getter_tag<Get>());
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	static void set(v8::Local<v8::String>, v8::Local<v8::Value>, v8::PropertyCallbackInfo<void> const&)
	{
		assert(false && "never should be called");
	}
};

template<typename Get, typename Set>
struct rw_property_impl<Get, Set, true>
	: r_property_impl<Get, Set, std::is_member_function_pointer<Get>::value>
{
	using Property = property_<Get, Set>;

	using class_type = typename std::tuple_element<0,
		typename function_traits<Set>::arguments>::type;

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String>,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info, setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

		v8::Isolate* isolate = info.GetIsolate();

		(obj.*set)(v8pp::from_v8<value_type>(isolate, value));
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String> name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info, direct_setter_tag)
	{
		(obj.*set)(name, value, info);
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String>, 
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info, isolate_setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();
		(obj.*set)(isolate, v8pp::from_v8<value_type>(isolate, value));
	}

	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		class_type& obj = v8pp::from_v8<class_type&>(isolate, info.This());

		Property prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			set_impl(obj, prop.set_, name, value, info, select_setter_tag<Set>());
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}
};

template<typename Get, typename Set>
struct rw_property_impl<Get, Set, false>
	: r_property_impl<Get, Set, std::is_member_function_pointer<Get>::value>
{
	using Property = property_<Get, Set>;

	static void set_impl(Set set, v8::Local<v8::String>, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info, setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

		v8::Isolate* isolate = info.GetIsolate();

		set(v8pp::from_v8<value_type>(isolate, value));
	}

	static void set_impl(Set set, v8::Local<v8::String> name, v8::Local<v8::Value> value, 
		v8::PropertyCallbackInfo<void> const& info, direct_setter_tag)
	{
		set(name, value, info);
	}

	static void set_impl(Set set, v8::Local<v8::String>, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info, isolate_setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();

		set(isolate, v8pp::from_v8<value_type>(isolate, value));
	}

	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			set_impl(prop.set_, name, value, info, select_setter_tag<Set>());
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}
};

} // namespace detail

/// Property with get and set functions
template<typename Get, typename Set>
struct property_ : detail::rw_property_impl<Get, Set, std::is_member_function_pointer<Set>::value>
{
	static_assert(detail::is_getter<Get>::value
		|| detail::is_direct_getter<Get>::value
		|| detail::is_isolate_getter<Get>::value,
		"property get function must be either `T ()` or "
		"`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
		"`T (v8::Isolate*)`");

	static_assert(detail::is_setter<Set>::value
		|| detail::is_direct_setter<Set>::value
		|| detail::is_isolate_setter<Set>::value,
		"property set function must be either `void (T)` or \
		`void (v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)` or \
		`void (v8::Isolate*, T)`");

	Get get_;
	Set set_;

	enum { is_readonly = false };
};

/// Read-only property class specialization for get only method
template<typename Get>
struct property_<Get, Get> : detail::r_property_impl<Get, Get, std::is_member_function_pointer<Get>::value>
{
	static_assert(detail::is_getter<Get>::value
		|| detail::is_direct_getter<Get>::value
		|| detail::is_isolate_getter<Get>::value,
		"property get function must be either `T ()` or "
		"void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
		"`T (v8::Isolate*)`");

	Get get_;

	enum { is_readonly = true };
};

/// Create read/write property from get and set member functions
template<typename Get, typename Set>
property_<Get, Set> property(Get get, Set set)
{
	property_<Get, Set> prop;
	prop.get_ = get;
	prop.set_ = set;
	return prop;
}

/// Create read-only property from a get function
template<typename Get>
property_<Get, Get> property(Get get)
{
	property_<Get, Get> prop;
	prop.get_ = get;
	return prop;
}

} // namespace v8pp

#endif // V8PP_PROPERTY_HPP_INCLUDED
