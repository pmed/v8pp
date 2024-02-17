#include "v8pp/throw_ex.hpp"

namespace v8pp {

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, std::string_view message, exception_ctor ctor, v8::Local<v8::Value> exception_options)
{
	v8::Local<v8::String> msg = v8::String::NewFromUtf8(isolate, message.data(),
		v8::NewStringType::kNormal, static_cast<int>(message.size())).ToLocalChecked();

// if constexpr (exception_ctor_with_options) doesn't work win VC++ 2022
#if V8_MAJOR_VERSION > 11 || (V8_MAJOR_VERSION == 11 && V8_MINOR_VERSION >= 9)
	v8::Local<v8::Value> ex = ctor(msg, exception_options);
#else
	(void)exception_options;
	v8::Local<v8::Value> ex = ctor(msg);
#endif

	return isolate->ThrowException(ex);
}

V8PP_IMPL v8::Local<v8::Value> throw_error(v8::Isolate* isolate, std::string_view message, v8::Local<v8::Value> exception_options)
{
	return throw_ex(isolate, message, v8::Exception::Error, exception_options);
}

V8PP_IMPL v8::Local<v8::Value> throw_range_error(v8::Isolate* isolate, std::string_view message, v8::Local<v8::Value> exception_options)
{
	return throw_ex(isolate, message, v8::Exception::RangeError, exception_options);
}

V8PP_IMPL v8::Local<v8::Value> throw_reference_error(v8::Isolate* isolate, std::string_view message, v8::Local<v8::Value> exception_options)
{
	return throw_ex(isolate, message, v8::Exception::ReferenceError, exception_options);
}

V8PP_IMPL v8::Local<v8::Value> throw_syntax_error(v8::Isolate* isolate, std::string_view message, v8::Local<v8::Value> exception_options)
{
	return throw_ex(isolate, message, v8::Exception::SyntaxError, exception_options);
}

V8PP_IMPL v8::Local<v8::Value> throw_type_error(v8::Isolate* isolate, std::string_view message, v8::Local<v8::Value> exception_options)
{
	return throw_ex(isolate, message, v8::Exception::TypeError, exception_options);
}

} // namespace v8pp
