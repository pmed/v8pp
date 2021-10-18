#include <node.h>
#include <v8pp/module.hpp>
#include <v8pp/class.hpp>
#include <v8pp/function.hpp>
#include <v8pp/object.hpp>

#include "myobject.h"

using namespace v8;

Local<Object> CreateObject(const FunctionCallbackInfo<Value>& args) {
  MyObject* obj = new MyObject(args);
  return v8pp::class_<MyObject>::import_external(args.GetIsolate(), obj);
}

double Add(MyObject const& obj1, MyObject const& obj2) {
  return obj1.value() + obj2.value();
}

void InitAll(Local<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();

  v8pp::class_<MyObject> MyObject_class(isolate);

  v8pp::module addon(isolate);
  addon.set("MyObject", MyObject_class); 
  addon.set("createObject", &CreateObject);
  addon.set("add", &Add);
  exports->SetPrototype(isolate->GetCurrentContext(), addon.new_instance());
  node::AtExit([](void* param)
  {
      v8pp::cleanup(static_cast<Isolate*>(param));
  }, isolate);
}

NODE_MODULE(addon, InitAll)
