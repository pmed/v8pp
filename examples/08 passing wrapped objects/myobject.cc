#include "myobject.h"
#include <v8pp/convert.hpp>

using namespace v8;

MyObject::MyObject(const FunctionCallbackInfo<Value>& args) {
  value_ = v8pp::from_v8<double>(args.GetIsolate(), args[0], 0);
}
