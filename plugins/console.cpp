//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <iostream>
#include <v8pp/module.hpp>
#include <v8pp/config.hpp>

namespace console {

void log(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::HandleScope handle_scope(args.GetIsolate());

	for (int i = 0; i < args.Length(); ++i)
	{
		if (i > 0) std::cout << ' ';
		v8::String::Utf8Value const str(args.GetIsolate(), args[i]);
		std::cout <<  *str;
	}
	std::cout << std::endl;
}

v8::Local<v8::Value> init(v8::Isolate* isolate)
{
	v8pp::module m(isolate);
	m.set("log", &log);
	return m.new_instance();
}

} // namespace console

V8PP_PLUGIN_INIT(v8::Isolate* isolate)
{
	return console::init(isolate);
}
