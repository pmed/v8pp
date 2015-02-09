#include "myobject.h"
#include <v8pp/class.hpp>
#include <v8pp/module.hpp>

using namespace v8;

void MyObject::Init() {
  Isolate* isolate = Isolate::GetCurrent();

  // Prepare class binding
  v8pp::class_<MyObject> MyObject_class(isolate);

  // Prototype
  MyObject_class.set("plusOne", &MyObject::PlusOne);

  v8pp::module bindings(isolate);
  bindings.set("MyObject", MyObject_class);

  static Persistent<Object> bindings_(isolate, bindings.new_instance());
}

MyObject::MyObject(const FunctionCallbackInfo<Value>& args) {
  value_ = v8pp::from_v8<double>(args.GetIsolate(), args[0], 0);
}

double MyObject::PlusOne() {
  value_ += 1;
  return value_;
}
