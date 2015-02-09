#ifndef MYOBJECT_H
#define MYOBJECT_H

#include <v8.h>

class MyObject {
 public:
  explicit MyObject(const v8::FunctionCallbackInfo<v8::Value>& args);

  inline double value() const { return value_; }

 private:
  double value_;
};

#endif
