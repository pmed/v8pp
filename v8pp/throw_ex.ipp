#include "v8pp/throw_ex.hpp"

namespace v8pp {

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, std::string_view message, exception_ctor ctor, v8::Local<v8::Value> exception_options)
{
	v8::Local<v8::String> msg = v8::String::NewFromUtf8(isolate, message.data(),
		v8::NewStringType::kNormal, static_cast<int>(message.size())).ToLocalChecked();

	v8::Local<v8::Value> ex;
	if (ctor)
	{
// if constexpr (exception_ctor_with_options) doesn't work win VC++ 2022
#if V8_MAJOR_VERSION > 11 || (V8_MAJOR_VERSION == 11 && V8_MINOR_VERSION >= 9)
		ex = ctor(msg, exception_options);
#else
		(void)exception_options;
		ex = ctor(msg);
#endif
	}
	else
	{
		ex = msg;
	}
	return isolate->ThrowException(ex);
}

} // namespace v8pp
