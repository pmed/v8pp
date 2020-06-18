#include "v8pp/throw_ex.hpp"

namespace v8pp {

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, std::string_view str)
{
	v8::Local<v8::String> msg = v8::String::NewFromUtf8(isolate, str.data(),
		v8::NewStringType::kNormal, static_cast<int>(str.size())).ToLocalChecked();
	return isolate->ThrowException(msg);
}

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, std::string_view str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>))
{
	v8::Local<v8::String> msg = v8::String::NewFromUtf8(isolate, str.data(),
		v8::NewStringType::kNormal, static_cast<int>(str.size())).ToLocalChecked();
	return isolate->ThrowException(exception_ctor(msg));
}

} // namespace v8pp
