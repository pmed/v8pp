#include "v8pp/throw_ex.hpp"
#include "v8pp/convert.hpp"

namespace v8pp {

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, string_view str)
{
	return isolate->ThrowException(to_v8(isolate, str));
}

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, string_view str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>))
{
	return isolate->ThrowException(exception_ctor(to_v8(isolate, str)));
}

} // namespace v8pp
