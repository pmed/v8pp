//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_CONTEXT_HPP_INCLUDED
#define V8PP_CONTEXT_HPP_INCLUDED

#include <string>
#include <map>

#include <v8.h>

#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"

namespace v8pp {

class module;

template<typename T, typename Traits>
class class_;

/// V8 isolate and context wrapper
class context
{
public:
	struct options
	{
		v8::Isolate* isolate = nullptr;
		v8::ArrayBuffer::Allocator* allocator = nullptr;
		bool add_default_global_methods = true;
		bool enter_context = true;
	};

	/// Create context with optional existing v8::Isolate
	/// and v8::ArrayBuffer::Allocator,
	/// and add default global methods (`require()`, `run()`)
	/// and enter the created v8 context
	explicit context(v8::Isolate* isolate = nullptr,
		v8::ArrayBuffer::Allocator* allocator = nullptr,
		bool add_default_global_methods = true,
		bool enter_context = true);

	explicit context(options const& opts)
		: context(opts.isolate, opts.allocator, opts.add_default_global_methods, opts.enter_context)
	{
	}

	~context();

	/// V8 isolate associated with this context
	v8::Isolate* isolate() { return isolate_; }

	/// V8 context implementation
	v8::Local<v8::Context> impl() { return to_local(isolate_, impl_); }

	/// Global object in this context
	v8::Local<v8::Object> global() { return impl()->Global(); }

	/// Library search path
	std::string const& lib_path() const { return lib_path_; }

	/// Set new library search path
	void set_lib_path(std::string const& lib_path) { lib_path_ = lib_path; }

	/// Run script file, returns script result
	/// or empty handle on failure, use v8::TryCatch around it to find out why.
	/// Must be invoked in a v8::HandleScope
	v8::Local<v8::Value> run_file(std::string const& filename);

	/// The same as run_file but uses string as the script source
	v8::Local<v8::Value> run_script(std::string_view source, std::string_view filename = "");

	/// Set a V8 value in the context global object with specified name
	context& value(std::string_view name, v8::Local<v8::Value> value);

	/// Set module to the context global object
	context& module(std::string_view name, v8pp::module& m);

	/// Set functions to the context global object
	template<typename Function, typename Traits = raw_ptr_traits>
	context& function(std::string_view name, Function&& func)
	{
		using Fun = typename std::decay<Function>::type;
		static_assert(detail::is_callable<Fun>::value, "Function must be callable");
		return value(name, wrap_function<Function, Traits>(isolate_, name, std::forward<Function>(func)));
	}

	/// Set class to the context global object
	template<typename T, typename Traits>
	context& class_(std::string_view name, v8pp::class_<T, Traits>& cl)
	{
		v8::HandleScope scope(isolate_);
		cl.class_function_template()->SetClassName(v8pp::to_v8(isolate_, name));
		return value(name, cl.js_function_template()->GetFunction(isolate_->GetCurrentContext()).ToLocalChecked());
	}

private:
	bool own_isolate_;
	bool enter_context_;
	v8::Isolate* isolate_;
	v8::Global<v8::Context> impl_;

	static void load_module(v8::FunctionCallbackInfo<v8::Value> const& args);
	static void run_file(v8::FunctionCallbackInfo<v8::Value> const& args);

	struct dynamic_module;
	std::map<std::string, dynamic_module> modules_;
	std::string lib_path_;
};

} // namespace v8pp

#endif // V8PP_CONTEXT_HPP_INCLUDED
