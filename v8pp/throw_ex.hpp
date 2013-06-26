#ifndef V8PP_THROW_EX_HPP_INCLUDED
#define V8PP_THROW_EX_HPP_INCLUDED

#include <string>

#include <v8.h>

namespace v8pp {

inline v8::Handle<v8::Primitive> throw_ex(char const* str)
{
	v8::ThrowException(v8::String::New(str));
	return v8::Undefined();
}

inline v8::Handle<v8::Primitive> throw_ex(std::string const& str)
{
	return throw_ex(str.c_str());
}

} // namespace v8pp

#endif // V8PP_THROW_EX_HPP_INCLUDED
