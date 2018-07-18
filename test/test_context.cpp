//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/context.hpp"
#include "v8pp/object.hpp"

#include "test.hpp"

void test_context()
{
	{
		v8pp::context context;

		v8::HandleScope scope(context.isolate());
		v8::Local<v8::Value> result = context.run_script("42");
		int const r = result->Int32Value(context.isolate()->GetCurrentContext()).FromJust();
		check_eq("run_script", r, 42);

		v8::TryCatch try_catch(context.isolate());
		result = context.run_script("syntax error");
		check("script with syntax error", result.IsEmpty());
		check(" has caught", try_catch.HasCaught());
	}

	{
		v8::Isolate* isolate = nullptr;
		v8::ArrayBuffer::Allocator* allocator = nullptr;
		bool add_default_global_methods = false;
		v8pp::context context(isolate, allocator, add_default_global_methods);

		v8::HandleScope scope(context.isolate());
		v8::Local<v8::Object> global = context.isolate()->GetCurrentContext()->Global();
		v8::Local<v8::Value> value;
		check("no global require", !v8pp::get_option(context.isolate(), global, "require", value));
		check("no global run", !v8pp::get_option(context.isolate(), global, "run", value));

		int const r = context.run_script("'4' + 2")->Int32Value(context.isolate()->GetCurrentContext()).FromJust();

		check_eq("run_script", r, 42);
	}
}
