#include "v8pp/throw_ex.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace v8pp {

V8PP_IMPL v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, char const* str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>))
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::String> message;
#ifdef _WIN32
	int const len = ::MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	if (len > 0)
	{
		std::vector<wchar_t> buf(len);
		::MultiByteToWideChar(CP_ACP, 0, str, -1, &buf[0], len);
		uint16_t const* data = reinterpret_cast<uint16_t const*>(&buf[0]);
		message = v8::String::NewFromTwoByte(isolate, data,
			v8::NewStringType::kNormal, len - 1).ToLocalChecked();
	}
#else
	message = v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal).ToLocalChecked();
#endif
	return scope.Escape(isolate->ThrowException(exception_ctor(message)));
}

} // namespace v8pp
