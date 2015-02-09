#include <node.h>
#include <v8pp/class.hpp>
#include <v8pp/function.hpp>
#include <v8pp/object.hpp>

#include "myobject.h"

using namespace v8;

static Handle<Object> CreateObject(const FunctionCallbackInfo<Value>& args) {
  MyObject* obj = new MyObject(args);
  return v8pp::class_<MyObject>::import_external(args.GetIsolate(), obj);
}

void InitAll(Handle<Object> exports, Handle<Object> module) {
  MyObject::Init();

  Isolate* isolate = Isolate::GetCurrent();
  v8pp::set_option(isolate, module, "exports", v8pp::wrap_function(isolate, "exports", &CreateObject));
}

NODE_MODULE(addon, InitAll)
