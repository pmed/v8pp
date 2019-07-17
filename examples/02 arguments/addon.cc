//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <node.h>
#include <v8pp/module.hpp>

using namespace v8;

double Add(double arg1, double arg2) {
  return arg1 + arg2;
}

void Init(Local<Object> exports) {
  v8pp::module addon(Isolate::GetCurrent());
  addon.set("add", &Add);
  exports->SetPrototype(Isolate::GetCurrent()->GetCurrentContext(), addon.new_instance());
}

NODE_MODULE(addon, Init)
