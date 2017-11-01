//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "myobject.h"
#include <v8pp/class.hpp>
#include <v8pp/module.hpp>

using namespace v8;

void MyObject::Init() {
  Isolate* isolate = Isolate::GetCurrent();

  // Prepare class binding
  v8pp::class_<MyObject> MyObject_class(isolate);

  // Prototype
  MyObject_class.function("plusOne", &MyObject::PlusOne);

  v8pp::module bindings(isolate);
  bindings.class_("MyObject", MyObject_class);

  static Persistent<Object> bindings_(isolate, bindings.new_instance());
}

MyObject::MyObject(const FunctionCallbackInfo<Value>& args) {
  value_ = v8pp::from_v8<double>(args.GetIsolate(), args[0], 0);
}

double MyObject::PlusOne() {
  value_ += 1;
  return value_;
}
