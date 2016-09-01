# Exceptions

The library allows to throw exceptions from C++ code. Such exceptions will be
converted to `v8::Exception::Error` and can be catched in JavaScript code.

It is also possible to throw a V8 exception with
`v8pp::throw_ex(v8::Isolate* isolate, char const* str, exception_ctor)`
function declared in a [`v8pp/throw_ex.hpp`](../v8pp/throw_ex.hpp) header,
where `exception_ctor` is a `v8::Exception` constructor:

  * `v8::Exception::Error` (default)
  * `v8::Exception::RangeError`
  * `v8::Exception::ReferenceError`
  * `v8::Exception::SyntaxError`
  * `v8::Exception::TypeError`


```c++
v8::Isolate* isolate = v8::Isolate::GetCurrent();

v8::Local<v8::Value> ex = v8pp::throw_ex(isolate, "my error message");
```
