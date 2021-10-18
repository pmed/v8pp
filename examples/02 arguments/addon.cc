#include <node.h>
#include <v8pp/module.hpp>

using namespace v8;

double Add(double arg1, double arg2) {
  return arg1 + arg2;
}

void Init(Local<Object> exports) {
  v8pp::module addon(Isolate::GetCurrent());
  addon.set("add", &Add);
  exports->SetPrototype(Isolate::GetCurrent()->GetCurrentContext(), addon.new_instance());
}

NODE_MODULE(addon, Init)
