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
#include "v8pp/ptr_traits.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/utility.hpp"

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
class external_data
{
public:
	static v8::Local<v8::External> set(v8::Isolate* isolate, T&& data)
	{
		external_data* value = new external_data;
		try
		{
			new (value->storage()) T(std::forward<T>(data));
		}
		catch (...)
		{
			delete value;
			throw;
		}

		v8::Local<v8::External> ext = v8::External::New(isolate, value);
		value->pext_.Reset(isolate, ext);
		value->pext_.SetWeak(value,
			[](v8::WeakCallbackInfo<external_data> const& data)
		{
			delete data.GetParameter();
		}, v8::WeakCallbackType::kParameter);
		return ext;
	}

	static T& get(v8::Local<v8::External> ext)
	{
		external_data* value = static_cast<external_data*>(ext->Value());
		return *static_cast<T*>(value->storage());
	}

private:
	void* storage() { return &storage_; }
	~external_data()
	{
		if (!pext_.IsEmpty())
		{
			static_cast<T*>(storage())->~T();
			pext_.Reset();
		}
	}
	using data_storage = typename std::aligned_storage<sizeof(T)>::type;
	data_storage storage_;
	v8::Global<v8::External> pext_;
};

template<typename T>
typename std::enable_if<is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
set_external_data(v8::Isolate* isolate, T value)
{
	return v8::External::New(isolate, pointer_cast<T>(value));
}

template<typename T>
typename std::enable_if<!is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
set_external_data(v8::Isolate* isolate, T&& value)
{
	return external_data<T>::set(isolate, std::forward<T>(value));
}

template<typename T>
typename std::enable_if<is_pointer_cast_allowed<T>::value, T>::type
get_external_data(v8::Local<v8::Value> value)
{
	return pointer_cast<T>(value.As<v8::External>()->Value());
}

template<typename T>
typename std::enable_if<!is_pointer_cast_allowed<T>::value, T&>::type
get_external_data(v8::Local<v8::Value> value)
{
	return external_data<T>::get(value.As<v8::External>());
}

template<typename Traits, typename F>
typename function_traits<F>::return_type
invoke(v8::FunctionCallbackInfo<v8::Value> const& args, std::false_type /*is_member_function_pointer*/)
{
	return call_from_v8<Traits, F>(std::forward<F>(get_external_data<F>(args.Data())), args);
}

template<typename Traits, typename F>
typename function_traits<F>::return_type
invoke(v8::FunctionCallbackInfo<v8::Value> const& args, std::true_type /*is_member_function_pointer*/)
{
	using arguments = typename function_traits<F>::arguments;
	static_assert(std::tuple_size<arguments>::value > 0, "");
	using class_type = typename std::decay<
		typename std::tuple_element<0, arguments>::type>::type;

	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Object> obj = args.This();
	auto ptr = class_<class_type, Traits>::unwrap_object(isolate, obj);
	if (!ptr)
	{
		throw std::runtime_error("method called on null instance");
	}
	return call_from_v8<Traits, class_type, F>(*ptr, std::forward<F>(get_external_data<F>(args.Data())), args);
}

template<typename Traits, typename F>
void forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, std::true_type /*is_void_return*/)
{
	invoke<Traits, F>(args, std::is_member_function_pointer<F>());
}

template<typename Traits, typename F>
void forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, std::false_type /*is_void_return*/)
{
	using return_type = typename function_traits<F>::return_type;
	using converter = typename call_from_v8_traits<F>::template arg_converter<return_type, Traits>;
	args.GetReturnValue().Set(converter::to_v8(args.GetIsolate(),
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
template<typename F, typename Traits = raw_ptr_traits>
v8::Local<v8::FunctionTemplate> wrap_function_template(v8::Isolate* isolate, F&& func)
{
	using F_type = typename std::decay<F>::type;
	return v8::FunctionTemplate::New(isolate,
		&detail::forward_function<Traits, F_type>,
		detail::set_external_data(isolate, std::forward<F_type>(func)));
}

/// Wrap C++ function into new V8 function
/// Set nullptr or empty string for name
/// to make the function anonymous
template<typename F, typename Traits = raw_ptr_traits>
v8::Local<v8::Function> wrap_function(v8::Isolate* isolate,
	char const* name, F && func)
{
	using F_type = typename std::decay<F>::type;
	v8::Local<v8::Function> fn = v8::Function::New(isolate->GetCurrentContext(),
		&detail::forward_function<Traits, F_type>,
		detail::set_external_data(isolate, std::forward<F_type>(func))).ToLocalChecked();
	if (name && *name)
	{
		fn->SetName(to_v8(isolate, name));
	}
	return fn;
}

} // namespace v8pp

#endif // V8PP_FUNCTION_HPP_INCLUDED
