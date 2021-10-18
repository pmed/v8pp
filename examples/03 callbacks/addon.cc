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
