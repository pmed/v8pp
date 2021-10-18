#ifndef MYOBJECT_H
#define MYOBJECT_H

#include <v8.h>

class MyObject {
 public:
  static void Init(v8::Local<v8::Object> exports);

  explicit MyObject(const v8::FunctionCallbackInfo<v8::Value>& args);

 private:
  double PlusOne();
  double value_;
};

#endif
