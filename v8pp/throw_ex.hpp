#ifndef V8PP_THROW_EX_HPP_INCLUDED
#define V8PP_THROW_EX_HPP_INCLUDED

#include <string>

#include <v8.h>

#ifdef WIN32
#include <windows.h>
#endif

namespace v8pp {

inline v8::Handle<v8::Value> throw_ex(char const* str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Handle<v8::String>) = v8::Exception::Error)
{
#ifdef _WIN32
	int const len = ::MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	if (len > 0)
	{
		std::vector<wchar_t> buf(len);
		::MultiByteToWideChar(CP_ACP, 0, str, -1, &buf[0], len);
		v8::Handle<v8::Value> exception = exception_ctor(v8::String::New(reinterpret_cast<uint16_t const*>(&buf[0]), len - 1));
		return v8::ThrowException(exception);
	}
#endif
	v8::Handle<v8::Value> exception = exception_ctor(v8::String::New(str));
	return v8::ThrowException(exception);
}

inline v8::Handle<v8::Value> throw_ex(std::string const& str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Handle<v8::String>) = v8::Exception::Error)
{
	return throw_ex(str.c_str(), exception_ctor);
}

} // namespace v8pp

#endif // V8PP_THROW_EX_HPP_INCLUDED
