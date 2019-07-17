//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <node.h>

#include <v8pp/call_v8.hpp>
#include <v8pp/function.hpp>
#include <v8pp/object.hpp>

using namespace v8;

void RunCallback(Local<Function> cb) {
  Isolate* isolate = Isolate::GetCurrent();

  v8pp::call_v8(isolate, cb, isolate->GetCurrentContext()->Global(), "hello world");
}

void Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = Isolate::GetCurrent();
  v8pp::set_option(isolate, module, "exports", v8pp::wrap_function(isolate, "exports", &RunCallback));
}

NODE_MODULE(addon, Init)
