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

#include <cstring> // for memcpy

#include <tuple>
#include <type_traits>

#include "v8pp/call_from_v8.hpp"
#include "v8pp/ptr_traits.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/utility.hpp"

namespace v8pp { namespace detail {

class external_data
{
public:
	template<typename T>
	using is_bitcast_allowed = std::integral_constant<bool,
		sizeof(T) <= sizeof(void*) &&
		std::is_default_constructible<T>::value &&
		std::is_trivially_copyable<T>::value>;

	template<typename T, typename = std::enable_if_t<is_bitcast_allowed<T>::value>>
	static v8::Local<v8::Value> set(v8::Isolate* isolate, T && value)
	{
		void* ptr;
		memcpy(&ptr, &value, sizeof value);
		return v8::External::New(isolate, ptr);
	}

	template<typename T, typename = std::enable_if_t<!is_bitcast_allowed<T>::value>>
	static v8::Local<v8::External> set(v8::Isolate* isolate, T&& data)
	{
		using ExtValue = value<T>;
		std::unique_ptr<ExtValue> value(new ExtValue);
		new (value->data()) T(std::forward<T>(data));

		v8::Local<v8::External> ext = v8::External::New(isolate, value.get());
		value->pext_.Reset(isolate, ext);
		value->pext_.SetWeak(value.get(),
			[](v8::WeakCallbackInfo<ExtValue> const& data)
		{
			std::unique_ptr<ExtValue> value(data.GetParameter());
			if (!value->pext_.IsEmpty())
			{
				value->data()->~T();
				value->pext_.Reset();
			}
		}, v8::WeakCallbackType::kParameter);

		value.release();
		return ext;
	}

	template<typename T, typename = std::enable_if_t<is_bitcast_allowed<T>::value>>
	static T get(v8::Local<v8::Value> value)
	{
		void* ptr = value.As<v8::External>()->Value();
		T data;
		memcpy(&data, &ptr, sizeof data);
		return data;
	}

	template<typename T, typename = std::enable_if_t<!is_bitcast_allowed<T>::value>>
	static T& get(v8::Local<v8::Value> ext)
	{
		using ExtValue = value<T>;
		ExtValue* value = static_cast<ExtValue*>(ext.As<v8::External>()->Value());
		return *value->data();
	}

private:
	template<typename T>
	struct value
	{
		T* data() { return static_cast<T*>(static_cast<void*>(&storage_)); }

		std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
		v8::Global<v8::External> pext_;
	};
};

template<typename Traits, typename F>
typename function_traits<F>::return_type
invoke(v8::FunctionCallbackInfo<v8::Value> const& args, std::false_type /*is_member_function_pointer*/)
{
	return call_from_v8<Traits, F>(std::forward<F>(external_data::get<F>(args.Data())), args);
}

template<typename Traits, typename F>
typename function_traits<F>::return_type
invoke(v8::FunctionCallbackInfo<v8::Value> const& args, std::true_type /*is_member_function_pointer*/)
{
	using class_type = typename std::decay<typename function_traits<F>::class_type>::type;

	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Object> obj = args.This();
	auto ptr = class_<class_type, Traits>::unwrap_object(isolate, obj);
	if (!ptr)
	{
		throw std::runtime_error("method called on null instance");
	}
	return call_from_v8<Traits, class_type, F>(*ptr, std::forward<F>(external_data::get<F>(args.Data())), args);
}

template<typename Traits, typename F>
void forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, std::true_type /*is_void_return*/)
{
	invoke<Traits, F>(args, std::is_member_function_pointer<F>());
}

template<typename Traits, typename F>
void forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, std::false_type /*is_void_return*/)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(),
		invoke<Traits, F>(args, std::is_member_function_pointer<F>())));
}

template<typename Traits, typename F>
void forward_function(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	static_assert(is_callable<F>::value || std::is_member_function_pointer<F>::value,
		"required callable F");

	v8::Isolate* isolate = args.GetIsolate();
	v8::HandleScope scope(isolate);

	try
	{
		forward_ret<Traits, F>(args, is_void_return<F>());
	}
	catch (std::exception const& ex)
	{
		args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
	}
}

} // namespace detail

/// Wrap C++ function into new V8 function template
template<typename Traits, typename F>
v8::Local<v8::FunctionTemplate> wrap_function_template(v8::Isolate* isolate, F&& func)
{
	using F_type = typename std::decay<F>::type;
	return v8::FunctionTemplate::New(isolate,
		&detail::forward_function<Traits, F_type>,
		detail::external_data::set(isolate, std::forward<F_type>(func)));
}

template<typename F>
v8::Handle<v8::FunctionTemplate> wrap_function_template(v8::Isolate* isolate, F&& func)
{
	return wrap_function_template<raw_ptr_traits>(isolate, std::forward<F>(func));
}

/// Wrap C++ function into new V8 function
/// Set nullptr or empty string for name
/// to make the function anonymous
template<typename Traits, typename F>
v8::Local<v8::Function> wrap_function(v8::Isolate* isolate,
	string_view const& name, F&& func)
{
	using F_type = typename std::decay<F>::type;
	v8::Local<v8::Function> fn = v8::Function::New(isolate->GetCurrentContext(),
		&detail::forward_function<Traits, F_type>,
		detail::external_data::set(isolate, std::forward<F_type>(func))).ToLocalChecked();
	if (!name.empty())
	{
		fn->SetName(to_v8(isolate, name));
	}
	return fn;
}

template<typename F>
v8::Handle<v8::Function> wrap_function(v8::Isolate* isolate,
	string_view const& name, F&& func)
{
	return wrap_function<raw_ptr_traits>(isolate, name, std::forward<F>(func));
}

} // namespace v8pp

#endif // V8PP_FUNCTION_HPP_INCLUDED
