#include <node.h>
#include <v8pp/function.hpp>
#include <v8pp/object.hpp>

using namespace v8;

std::string MyFunction() {
  return "hello world";
}

Handle<Function> CreateFunction(Isolate* isolate) {
  EscapableHandleScope scope(isolate);

  Local<FunctionTemplate> tpl = v8pp::wrap_function_template(isolate, &MyFunction);
  Local<Function> fn = tpl->GetFunction();

  // omit this to make it anonymous
  fn->SetName(v8pp::to_v8(isolate, "theFunction"));
  return scope.Escape(fn);
}

void Init(Handle<Object> exports, Handle<Object> module) {
  Isolate* isolate = Isolate::GetCurrent();
  v8pp::set_option(isolate, module, "exports", v8pp::wrap_function(isolate, "exports", &CreateFunction));
}

NODE_MODULE(addon, Init)