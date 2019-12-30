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

template<typename Attribute, typename Get, typename Set>
struct member_property_;

namespace detail {

struct getter_tag {};
struct direct_getter_tag {};
struct isolate_getter_tag {};
struct object_getter_tag {};

struct setter_tag {};
struct direct_setter_tag {};
struct isolate_setter_tag {};
struct object_setter_tag {};

template<typename F>
using is_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 0 && !is_void_return<F>::value>;

template<typename F> 
using is_object_getter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 1 &&
	std::is_reference<typename call_from_v8_traits<F>::template arg_type<0>>::value && 
	std::is_const<typename std::remove_reference<typename call_from_v8_traits<F>::template arg_type<0>>::type>::value &&	
	!is_void_return<F>::value>;

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
using is_object_setter = std::integral_constant<bool,
	call_from_v8_traits<F>::arg_count == 2 &&
	std::is_reference<typename call_from_v8_traits<F>::template arg_type<0>>::value && 
	!std::is_const<typename std::remove_reference<typename call_from_v8_traits<F>::template arg_type<0>>::type>::value &&	
	is_void_return<F>::value>;	

template<typename F>
using is_loose_setter = std::integral_constant<bool,
    call_from_v8_traits<F>::arg_count == 1 &&
    std::is_member_function_pointer<F>::value &&
    std::is_same<
            typename std::decay<typename function_traits<F>::return_type>::type,
            typename std::decay<typename std::tuple_element<0, typename function_traits<F>::arguments>::type>::type>::value>;

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
		isolate_getter_tag, 
	//typename std::conditional<is_object_getter<F>::value, 
	//	object_getter_tag, 
	getter_tag>::type
>::type;

template<typename F>
using select_setter_tag = typename std::conditional<is_direct_setter<F>::value,
	direct_setter_tag,
	typename std::conditional<is_isolate_setter<F>::value,
		isolate_setter_tag, 
//	typename std::conditional<is_object_setter<F>::value,
//		object_setter_tag, 
		setter_tag>::type
>::type;

template<typename Get, typename Set, bool get_is_mem_fun>
struct r_property_impl;

template<typename Get, typename Set, bool set_is_mem_fun>
struct rw_property_impl;

template<typename Get, typename Set>
struct r_property_impl<Get, Set, true>
{
	using property_type = property_<Get, Set>;

	using class_type = typename std::decay<typename std::tuple_element<0,
		typename function_traits<Get>::arguments> ::type>::type;

	static_assert(is_getter<Get>::value
		|| is_direct_getter<Get>::value
		|| is_isolate_getter<Get>::value,
		"property get function must be either `T ()` or \
		`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or \
		`T (v8::Isolate*)`");

	static void get_impl(class_type& obj, Get get, v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info, getter_tag)
	{
		info.GetReturnValue().Set(to_v8(info.GetIsolate(), (obj.*get)()));
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

	template<typename Traits>
	static void get(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	try
	{
		auto obj = v8pp::class_<class_type, Traits>::unwrap_object(info.GetIsolate(), info.This());
		assert(obj);

		property_type const& prop = detail::get_external_data<property_type>(info.Data());
		assert(prop.getter);

		if (obj && prop.getter)
		{
			get_impl(*obj, prop.getter, name, info, select_getter_tag<Get>());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}

	template<typename Traits>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value>,
		v8::PropertyCallbackInfo<void> const& info)
	{
		assert(false && "read-only property");
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(),
			"read-only property " + from_v8<std::string>(info.GetIsolate(), name)));
	}
};

template<typename Get, typename Set>
struct r_property_impl<Get, Set, false>
{
	using property_type = property_<Get, Set>;

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

	template<class Traits>
	static void get_with_object(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	try {
		using class_type = typename std::decay<typename std::tuple_element<0,
			typename function_traits<Get>::arguments>::type>::type;
		auto obj = v8pp::class_<class_type, Traits>::unwrap_object(info.GetIsolate(), info.This());
		assert(obj);

		property_type const& prop = detail::get_external_data<property_type>(info.Data());
		assert(function_traits<Get>::is_not_empty(prop.getter));

		if (function_traits<Get>::is_not_empty(prop.getter))
		{
			info.GetReturnValue().Set(to_v8(info.GetIsolate(), prop.getter(*obj)));
		}
	} 
	catch (std::exception const& ex) 
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}


	static void set(v8::Local<v8::String> name, v8::Local<v8::Value>,
		v8::PropertyCallbackInfo<void> const& info)
	{
		assert(false && "read-only property");
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(),
			"read-only property " + from_v8<std::string>(info.GetIsolate(), name)));
	}
};

template<typename Get, typename Set>
struct rw_property_impl<Get, Set, true>
	: r_property_impl<Get, Set, std::is_member_function_pointer<Get>::value>
{
	using property_type = property_<Get, Set>;

	using class_type = typename std::decay<typename std::tuple_element<0,
		typename function_traits<Set>::arguments>::type>::type;

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String>,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

		(obj.*set)(v8pp::from_v8<value_type>(info.GetIsolate(), value));
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String> name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		direct_setter_tag)
	{
		(obj.*set)(name, value, info);
	}

	static void set_impl(class_type& obj, Set set, v8::Local<v8::String>, 
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		isolate_setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();

		(obj.*set)(isolate, v8pp::from_v8<value_type>(isolate, value));
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
			set_impl(*obj, prop.setter, name, value, info, select_setter_tag<Set>());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}
};

template<typename Get, typename Set>
struct rw_property_impl<Get, Set, false>
	: r_property_impl<Get, Set, std::is_member_function_pointer<Get>::value>
{
	using property_type = property_<Get, Set>;

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

	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info)
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

	template<class Traits>
	static void set_with_object(v8::Local<v8::String> name,
		v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	try
	{
		using class_type = typename std::decay<typename std::tuple_element<0,
			typename function_traits<Set>::arguments>::type>::type;
		auto obj = v8pp::class_<class_type, Traits>::unwrap_object(info.GetIsolate(), info.This());
		assert(obj);
		property_type const& prop = detail::get_external_data<property_type>(info.Data());
		assert(function_traits<Set>::is_not_empty(prop.setter));

		if (function_traits<Set>::is_not_empty(prop.setter))
		{
			using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;
			prop.setter(*obj, v8pp::from_v8<value_type>(info.GetIsolate(), value));
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}

};

template <typename Attribute, typename Get, typename Set>
struct rw_member_property {
	using property_type = member_property_<Attribute, Get, Set>;

	using class_type = typename std::decay<typename std::tuple_element<0,
			typename function_traits<Attribute>::arguments> ::type>::type;

    static_assert(std::is_member_pointer<Attribute>::value, "member property must be `&T::member`");

    static_assert(is_getter<Get>::value
				  || is_direct_getter<Get>::value
				  || is_isolate_getter<Get>::value,
				  "property get function must be either `T ()` or \
		`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or \
		`T (v8::Isolate*)`");

	static void get_impl(class_type& obj, Attribute attr, Get get, v8::Local<v8::String>,
						 v8::PropertyCallbackInfo<v8::Value> const& info, getter_tag)
	{
		info.GetReturnValue().Set(to_v8(info.GetIsolate(), ((obj.*attr).*get)()));
	}

	static void get_impl(class_type& obj, Attribute attr, Get get,
						 v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
						 direct_getter_tag)
	{
		((obj.*attr).*get)(name, info);
	}

	static void get_impl(class_type& obj, Attribute attr, Get get, v8::Local<v8::String>,
						 v8::PropertyCallbackInfo<v8::Value> const& info, isolate_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		info.GetReturnValue().Set(to_v8(isolate, ((obj.*attr).*get)(isolate)));
	}

	template<typename Traits>
	static void get(v8::Local<v8::String> name,
					v8::PropertyCallbackInfo<v8::Value> const& info)
	try
	{
		auto obj = v8pp::class_<class_type, Traits>::unwrap_object(info.GetIsolate(), info.This());
		assert(obj);

		property_type const& prop = detail::get_external_data<property_type>(info.Data());
		assert(prop.getter);
        assert(prop.attr);

		if (obj && prop.getter && prop.attr)
		{
			get_impl(*obj, prop.attr, prop.getter, name, info, select_getter_tag<Get>());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}

	static void set_impl(class_type& obj, Attribute attr, Set set, v8::Local<v8::String>,
						 v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
						 setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<0>;

		((obj.*attr).*set)(v8pp::from_v8<value_type>(info.GetIsolate(), value));
	}

	static void set_impl(class_type& obj, Attribute attr, Set set, v8::Local<v8::String> name,
						 v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
						 direct_setter_tag)
	{
		((obj.*attr).*set)(name, value, info);
	}

	static void set_impl(class_type& obj, Attribute attr, Set set, v8::Local<v8::String>,
						 v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
						 isolate_setter_tag)
	{
		using value_type = typename call_from_v8_traits<Set>::template arg_type<1>;

		v8::Isolate* isolate = info.GetIsolate();

		((obj.*attr).*set)(isolate, v8pp::from_v8<value_type>(isolate, value));
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
			set_impl(*obj, prop.attr, prop.setter, name, value, info, select_setter_tag<Set>());
		}
	}
	catch (std::exception const& ex)
	{
		info.GetReturnValue().Set(throw_ex(info.GetIsolate(), ex.what()));
	}

};
} // namespace detail

/// Property with get and set functions
template<typename Get, typename Set>
struct property_
	: detail::rw_property_impl<Get, Set, std::is_member_function_pointer<Set>::value>
{
	static_assert(detail::is_getter<Get>::value
		|| detail::is_object_getter<Get>::value
		|| detail::is_direct_getter<Get>::value
		|| detail::is_isolate_getter<Get>::value,
		"property get function must be either `T ()` or "
		"`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
		"`T (v8::Isolate*)`");

	static_assert(detail::is_setter<Set>::value
		|| detail::is_object_setter<Set>::value
		|| detail::is_direct_setter<Set>::value
		|| detail::is_isolate_setter<Set>::value,
		"property set function must be either `void (T)` or \
		`void (v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)` or \
		`void (v8::Isolate*, T)`");

	Get getter;
	Set setter;

	enum { is_readonly = false };

	property_(Get getter, Set setter)
		: getter(getter)
		, setter(setter)
	{
	}

	template<typename OtherGet, typename OtherSet>
	property_(property_<OtherGet, OtherSet> const& other)
		: getter(other.getter)
		, setter(other.setter)
	{
	}
};

/// Read-only property class specialization for get only method
template<typename Get>
struct property_<Get, Get>
	: detail::r_property_impl<Get, Get, std::is_member_function_pointer<Get>::value>
{
	static_assert(detail::is_getter<Get>::value
		|| detail::is_object_getter<Get>::value
		|| detail::is_direct_getter<Get>::value
		|| detail::is_isolate_getter<Get>::value,
		"property get function must be either `T ()` or "
		"void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
		"`T (v8::Isolate*)`");

	Get getter;

	enum { is_readonly = true };

	explicit property_(Get getter)
		: getter(getter)
	{
	}

	template<typename OtherGet>
	explicit property_(property_<OtherGet, OtherGet> const& other)
		: getter(other.getter)
	{
	}
};

/// Property with get and set functions
template<typename Attribute, typename Get, typename Set>
struct member_property_
			: detail::rw_member_property<Attribute, Get, Set>
	{
        static_assert(std::is_member_pointer<Attribute>::value, "member property must be `&T::member`");

		static_assert(detail::is_getter<Get>::value
					  || detail::is_direct_getter<Get>::value
					  || detail::is_isolate_getter<Get>::value,
					  "property get function must be either `T ()` or "
					  "`void (v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)` or "
					  "`T (v8::Isolate*)`");

		static_assert(detail::is_loose_setter<Set>::value, "wtf?");

		static_assert(detail::is_setter<Set>::value
					  || detail::is_direct_setter<Set>::value
					  || detail::is_isolate_setter<Set>::value
					  || detail::is_loose_setter<Set>::value,
					  "property set function must be either `void (T)` or \
		`void (v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)` or \
		`void (v8::Isolate*, T)` or \
        `T& (T::()(const T2&)`"
       );

		Attribute attr;
		Get getter;
		Set setter;

		enum { is_readonly = false };

		member_property_(Attribute attr, Get getter, Set setter)
				: attr(attr)
				, getter(getter)
				, setter(setter)
		{
		}

		template<typename OtherAttribute, typename OtherGet, typename OtherSet>
		member_property_(member_property_<OtherAttribute, OtherGet, OtherSet> const& other)
				: attr(other.attr)
				, getter(other.getter)
				, setter(other.setter)
		{
		}
	};
/// Create read/write property from get and set member functions
template<typename Get, typename Set>
property_<Get, Set> property(Get get, Set set)
{
	return property_<Get, Set>(get, set);
}

/// Create read-only property from a get function
template<typename Get>
property_<Get, Get> property(Get get)
{
	return property_<Get, Get>(get);
}

template<typename Attribute, typename Get, typename Set>
member_property_<Attribute, Get, Set> property(Attribute attr, Get get, Set set) {
	return member_property_<Attribute, Get, Set>(attr, get, set);
}

} // namespace v8pp

#endif // V8PP_PROPERTY_HPP_INCLUDED
