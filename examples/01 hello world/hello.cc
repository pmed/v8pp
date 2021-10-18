#include <node.h>
#include <v8pp/module.hpp>

using namespace v8;

char const* Method() {
  return "world";
}

void init(Local<Object> exports) {
  v8pp::module addon(Isolate::GetCurrent());
  addon.set("hello", &Method);
  exports->SetPrototype(Isolate::GetCurrent()->GetCurrentContext(), addon.new_instance());
}

NODE_MODULE(addon, init)
