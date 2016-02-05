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
#include <v8pp/class.hpp>
#include <v8pp/function.hpp>
#include <v8pp/object.hpp>

#include "myobject.h"

using namespace v8;

Handle<Object> CreateObject(const FunctionCallbackInfo<Value>& args) {
  MyObject* obj = new MyObject(args);
  return v8pp::class_<MyObject>::import_external(args.GetIsolate(), obj);
}

double Add(MyObject const& obj1, MyObject const& obj2) {
  return obj1.value() + obj2.value();
}

void InitAll(Handle<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();

  v8pp::class_<MyObject> MyObject_class(isolate);

  v8pp::module addon(isolate);
  addon.set("MyObject", MyObject_class); 
  addon.set("createObject", &CreateObject);
  addon.set("add", &Add);
  exports->SetPrototype(addon.new_instance());
}

NODE_MODULE(addon, InitAll)
