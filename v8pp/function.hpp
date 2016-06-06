//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_FUNCTION_HPP_INCLUDED
#define V8PP_FUNCTION_HPP_INCLUDED

#include <tuple>
#include <type_traits>

#include "v8pp/call_from_v8.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/utility.hpp"

#if defined(V8_MAJOR_VERSION) && defined(V8_MINOR_VERSION) && defined(V8_BUILD_NUMBER) \
      && (V8_MAJOR_VERSION > 4 || (V8_MAJOR_VERSION == 4 \
      && (V8_MINOR_VERSION > 3 || (V8_MINOR_VERSION == 3 && V8_BUILD_NUMBER > 28))))
#define V8_USE_WEAK_CB_INFO
#endif


namespace v8pp {

namespace detail {

template<typename T>
using is_pointer_cast_allowed = std::integral_constant<bool,
	sizeof(T) <= sizeof(void*) && std::is_trivial<T>::value>;

template<typename T>
union pointer_cast
{
private:
	void* ptr;
	T value;

public:
	static_assert(is_pointer_cast_allowed<T>::value, "pointer_cast is not allowed");

	explicit pointer_cast(void* ptr) : ptr(ptr) {}
	explicit pointer_cast(T value) : value(value) {}

	operator void*() const { return ptr; }
	operator T() const { return value; }
};

template<typename T>
typename std::enable_if<is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
set_external_data(v8::Isolate* isolate, T value)
{
	return v8::External::New(isolate, pointer_cast<T>(value));
}

template<typename T>
typename std::enable_if<!is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
set_external_data(v8::Isolate* isolate, T const& value)
{
	T* data = new T(value);

	v8::Local<v8::External> ext = v8::External::New(isolate, data);

	v8::Persistent<v8::External> pext(isolate, ext);
	pext.SetWeak(data,
#ifdef V8_USE_WEAK_CB_INFO
		[](v8::WeakCallbackInfo<T> const& data)
#else
		[](v8::WeakCallbackData<v8::External, T> const& data)
#endif
		{
			delete data.GetParameter();
		}
#ifdef V8_USE_WEAK_CB_INFO
		,v8::WeakCallbackType::kParameter
#endif
		);

	return ext;
}

template<typename T>
typename std::enable_if<is_pointer_cast_allowed<T>::value, T>::type
get_external_data(v8::Handle<v8::Value> value)
{
	return pointer_cast<T>(value.As<v8::External>()->Value());
}

template<typename T>
typename std::enable_if<!is_pointer_cast_allowed<T>::value, T&>::type
get_external_data(v8::Handle<v8::Value> value)
{
	T* data = static_cast<T*>(value.As<v8::External>()->Value());
	return *data;
}

template<typename F>
typename std::enable_if<is_callable<F>::value,
	typename function_traits<F>::return_type>::type
invoke(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	F f = get_external_data<F>(args.Data());
	return call_from_v8(std::forward<F>(f), args);
}

template<typename F>
typename std::enable_if<std::is_member_function_pointer<F>::value,
	typename function_traits<F>::return_type>::type
invoke(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	using arguments = typename function_traits<F>::arguments;
	static_assert(std::tuple_size<arguments>::value > 0, "");
	using class_type = typename std::tuple_element<0, arguments>::type;

	F f = get_external_data<F>(args.Data());
	class_type& obj = from_v8<class_type&>(args.GetIsolate(), args.This());

	return call_from_v8(obj, std::forward<F>(f), args);
}

template<typename F>
typename std::enable_if<is_void_return<F>::value>::type
forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	invoke<F>(args);
}

template<typename F>
typename std::enable_if<!is_void_return<F>::value>::type
forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(), invoke<F>(args)));
}

template<typename F>
void forward_function(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	static_assert(is_callable<F>::value || std::is_member_function_pointer<F>::value,
		"required callable F");

	v8::Isolate* isolate = args.GetIsolate();
	v8::HandleScope scope(isolate);

	try
	{
		forward_ret<F>(args);
	}
	catch (std::exception const& ex)
	{
		args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
	}
}

} // namespace detail

/// Wrap C++ function into new V8 function template
template<typename F>
v8::Handle<v8::FunctionTemplate> wrap_function_template(v8::Isolate* isolate, F func)
{
	return v8::FunctionTemplate::New(isolate, &detail::forward_function<F>,
		detail::set_external_data(isolate, func));
}

/// Wrap C++ function into new V8 function
/// Set nullptr or empty string for name
/// to make the function anonymous
template<typename F>
v8::Handle<v8::Function> wrap_function(v8::Isolate* isolate, char const* name, F func)
{
	v8::Handle<v8::Function> fn = v8::Function::New(isolate, &detail::forward_function<F>,
		detail::set_external_data(isolate, func));
	if (name && *name)
	{
		fn->SetName(to_v8(isolate, name));
	}
	return fn;
}

} // namespace v8pp

#endif // V8PP_FUNCTION_HPP_INCLUDED
