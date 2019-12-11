#include "v8pp/throw_ex.hpp"

namespace v8pp {

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, char const* str)
{
	return isolate->ThrowException(v8::String::NewFromUtf8(isolate, str).ToLocalChecked());
}

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, char const* str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>))
{
	return isolate->ThrowException(exception_ctor(v8::String::NewFromUtf8(isolate, str).ToLocalChecked()));
}

} // namespace v8pp
