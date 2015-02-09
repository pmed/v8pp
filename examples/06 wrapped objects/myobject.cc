#include "myobject.h"
#include <v8pp/class.hpp>
#include <v8pp/module.hpp>

using namespace v8;

void MyObject::Init(Handle<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();

  // Prepare class binding
  v8pp::class_<MyObject> MyObject_class(isolate);

  // constructor signature
  MyObject_class.ctor<const FunctionCallbackInfo<Value>&>();

  // Prototype
  MyObject_class.set("plusOne", &MyObject::PlusOne);

  v8pp::module addon(isolate);
  addon.set("MyObject", MyObject_class);

  exports->SetPrototype(addon.new_instance());
}

MyObject::MyObject(const FunctionCallbackInfo<Value>& args) {
  value_ = v8pp::from_v8<double>(args.GetIsolate(), args[0], 0);
}

double MyObject::PlusOne() {
  value_ += 1;
  return value_;
}
