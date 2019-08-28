#include "v8pp/throw_ex.hpp"

namespace v8pp {

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, char const* str)
{
#if V8_MAJOR_VERSION >= 7 && V8_MINOR_VERSION >= 7
    return isolate->ThrowException(v8::String::NewFromUtf8(isolate, str).ToLocalChecked());
#else
    return isolate->ThrowException(v8::String::NewFromUtf8(isolate, str));
#endif
}

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, char const* str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>))
{
#if V8_MAJOR_VERSION >= 7 && V8_MINOR_VERSION >= 7
    return isolate->ThrowException(exception_ctor(v8::String::NewFromUtf8(isolate, str).ToLocalChecked()));
#else
    return isolate->ThrowException(exception_ctor(v8::String::NewFromUtf8(isolate, str)));
#endif
}

} // namespace v8pp
