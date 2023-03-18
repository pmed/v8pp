#include <node.h>
#include "myobject.h"

using namespace v8;

void InitAll(Local<Object> exports) {
  MyObject::Init(exports);
}

NODE_MODULE(addon, InitAll)
