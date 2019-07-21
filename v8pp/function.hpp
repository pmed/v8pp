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

#include <type_traits>

#include "v8pp/call_from_v8.hpp"
#include "v8pp/ptr_traits.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/utility.hpp"

namespace v8pp { namespace detail {

class external_data
{
public:
	//TODO: allow non-capturing lambdas
	template<typename T>
	static constexpr bool is_bitcast_allowed = sizeof(T) <= sizeof(void*) &&
		std::is_default_constructible_v<T> &&
		std::is_trivially_copyable_v<T>;

	template<typename T>
	static v8::Local<v8::Value> set(v8::Isolate* isolate, T && value)
	{
		if constexpr (is_bitcast_allowed<T>)
		{
			void* ptr;
			memcpy(&ptr, &value, sizeof value);
			return v8::External::New(isolate, ptr);
		}
		else
		{
			using ExtValue = value_holder<T>;
			std::unique_ptr<ExtValue> ext_value(new ExtValue);
			new (ext_value->data()) T(std::forward<T>(value));

			v8::Local<v8::External> ext = v8::External::New(isolate, ext_value.get());
			ext_value->pext_.Reset(isolate, ext);
			ext_value->pext_.SetWeak(ext_value.get(),
				[](v8::WeakCallbackInfo<ExtValue> const& data)
				{
					std::unique_ptr<ExtValue> ext_value(data.GetParameter());
					if (!ext_value->pext_.IsEmpty())
					{
						ext_value->data()->~T();
						ext_value->pext_.Reset();
					}
				}, v8::WeakCallbackType::kParameter);

			ext_value.release();
			return ext;
		}
	}

	template<typename T>
	static decltype(auto) get(v8::Local<v8::Value> value)
	{
		if constexpr (is_bitcast_allowed<T>)
		{
			void* ptr = value.As<v8::External>()->Value();
			T data;
			memcpy(&data, &ptr, sizeof data);
			return data;
		}
		else
		{
			using ExtValue = value_holder<T>;
			ExtValue* ext_value = static_cast<ExtValue*>(value.As<v8::External>()->Value());
			T& data = *(ext_value->data());
			return (data); // as reference
		}
	}
private:
	template<typename T>
	struct value_holder
	{
		T* data() { return static_cast<T*>(static_cast<void*>(&storage_)); }

		std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
		v8::Global<v8::External> pext_;
	};
};

template<typename Traits, typename F, typename FTraits>
decltype(auto) invoke(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if constexpr (std::is_member_function_pointer<F>())
	{
		using class_type = std::decay_t<typename FTraits::class_type>;
		auto obj = class_<class_type, Traits>::unwrap_object(args.GetIsolate(), args.This());
		if (!obj)
		{
			throw std::runtime_error("method called on null instance");
		}
		return (call_from_v8<Traits>(std::forward<F>(external_data::get<F>(args.Data())), args, *obj));
	}
	else
	{
		return (call_from_v8<Traits>(std::forward<F>(external_data::get<F>(args.Data())), args));
	}
}

template<typename Traits, typename F>
void forward_function(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	using FTraits = function_traits<F>;

	static_assert(is_callable<F>::value || std::is_member_function_pointer<F>::value,
		"required callable F");

	v8::Isolate* isolate = args.GetIsolate();
	v8::HandleScope scope(isolate);
	try
	{
		if constexpr (std::is_same_v<typename FTraits::return_type, void>)
		{
			invoke<Traits, F, FTraits>(args);
		}
		else
		{
			using return_type = typename FTraits::return_type;
			using converter = typename call_from_v8_traits<F>::template arg_converter<return_type, Traits>;
			args.GetReturnValue().Set(converter::to_v8(isolate, invoke<Traits, F, FTraits>(args)));
		}
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
		detail::external_data::set(isolate, std::forward<F_type>(func)));
}

/// Wrap C++ function into new V8 function
/// Set nullptr or empty string for name
/// to make the function anonymous
template<typename F, typename Traits = raw_ptr_traits>
v8::Local<v8::Function> wrap_function(v8::Isolate* isolate, std::string_view name, F&& func)
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

} // namespace v8pp

#endif // V8PP_FUNCTION_HPP_INCLUDED
