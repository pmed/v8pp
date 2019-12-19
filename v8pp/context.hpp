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

namespace v8pp {

class module;

template<typename T, typename Traits>
class class_;

/// V8 isolate and context wrapper
class context
{
public:

	/// Create context with optional existing v8::Isolate
	/// and v8::ArrayBuffer::Allocator,
	//  and add default global methods (`require()`, `run()`)
	explicit context(v8::Isolate* isolate = nullptr,
		v8::ArrayBuffer::Allocator* allocator = nullptr,
		bool add_default_global_methods = true,
        bool enter_context = true);
	~context();

	/// V8 isolate associated with this context
	v8::Isolate* isolate() { return isolate_; }

	/// Library search path
	std::string const& lib_path() const { return lib_path_; }

	/// Set new library search path
	void set_lib_path(std::string const& lib_path) { lib_path_ = lib_path; }

	/// Run script file, returns script result
	/// or empty handle on failure, use v8::TryCatch around it to find out why.
	/// Must be invoked in a v8::HandleScope
	v8::Local<v8::Value> run_file(std::string const& filename);

	/// The same as run_file but uses string as the script source
	v8::Local<v8::Value> run_script(std::string const& source,
		std::string const& filename = "");

	/// Set a V8 value in the context global object with specified name
	context& set(char const* name, v8::Local<v8::Value> value);

	/// Set module to the context global object
	context& set(char const *name, module& m);

	/// Set class to the context global object
	template<typename T, typename Traits>
	context& set(char const* name, class_<T, Traits>& cl)
	{
		v8::HandleScope scope(isolate_);
		cl.class_function_template()->SetClassName(v8pp::to_v8(isolate_, name));
		return set(name, cl.js_function_template()->GetFunction(isolate_->GetCurrentContext()).ToLocalChecked());
	}

	v8::Local<v8::Context> impl();
  
    /// Enter the context 
    void enter();

    /// Exit the context
    void exit();

    /// The context_scope class is an RAII guard to enter and exit the contex's scope. 
    /// In Multithreaded scenarios the users of the library must enter the context before running.
    /// Note, 
    /// 1. If the context owns the Isolate then this guard will also Enter the isolate.
    /// 2. If the context owns the Isolate then this will also aquire the Locker for this thread  
    class context_scope {
      public:
        context_scope(context &context);
        ~context_scope();
        context_scope(context_scope &&other) = delete;
        context_scope(const context_scope &other) = delete;
        context_scope &operator=(const context_scope &other) = delete;

      private:
        context &context_;
        struct empty_struct {};
        struct v8locks {
          v8::Locker locker;
          v8::Isolate::Scope scope;
        };
        bool owns_locks_;
        union {
          empty_struct empty_;
          v8locks locks_;
        };
    };

private:
    bool enter_context_;
	bool own_isolate_;
	v8::Isolate* isolate_;
	v8::Global<v8::Context> impl_;

	struct dynamic_module;
	using dynamic_modules = std::map<std::string, dynamic_module>;

	static void load_module(v8::FunctionCallbackInfo<v8::Value> const& args);
	static void run_file(v8::FunctionCallbackInfo<v8::Value> const& args);

	dynamic_modules modules_;
	std::string lib_path_;
};

} // namespace v8pp

#endif // V8PP_CONTEXT_HPP_INCLUDED
