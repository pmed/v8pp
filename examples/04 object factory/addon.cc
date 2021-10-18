#include <node.h>
#include <v8pp/object.hpp>
#include <v8pp/function.hpp>

#include <node.h>

using namespace v8;

Local<Object> CreateObject(v8::Isolate* isolate, std::string const& msg) {
  EscapableHandleScope scope(isolate);

  Local<Object> obj = Object::New(isolate);
  v8pp::set_option(isolate, obj, "msg", msg);
  return scope.Escape(obj);
}

void Init(Local<Object> exports, Local<Object> module) {
  Isolate* isolate = Isolate::GetCurrent();
  v8pp::set_option(isolate, module, "exports", v8pp::wrap_function(isolate, "exports", &CreateObject));
}

NODE_MODULE(addon, Init)
