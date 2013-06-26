#ifndef V8PP_TO_V8_HPP_INCLUDED
#define V8PP_TO_V8_HPP_INCLUDED

#include <v8.h>

#include <string>
#include <cstdint>

namespace v8pp {

inline v8::Handle<v8::Value> to_v8(v8::Handle<v8::Value> src)
{
	return src;
}

inline v8::Handle<v8::Value> to_v8(std::string const& src)
{
	return v8::String::New(src.c_str());
}

inline v8::Handle<v8::Value> to_v8(char const *src)
{
	return v8::String::New(src);
}

inline v8::Handle<v8::Value> to_v8(int64_t const src)
{
	// TODO: check bounds?
	return v8::Number::New(static_cast<double>(src));
}

inline v8::Handle<v8::Value> to_v8(uint64_t const src)
{
	// TODO: check bounds?
	return v8::Number::New(static_cast<double>(src));
}

inline v8::Handle<v8::Value> to_v8(float const src)
{
	return v8::Number::New(src);
}

inline v8::Handle<v8::Value> to_v8(double const src)
{
	return v8::Number::New(src);
}

inline v8::Handle<v8::Value> to_v8(int32_t const src)
{
	return v8::Int32::New(src);
}

inline v8::Handle<v8::Value> to_v8(uint32_t const src)
{
	return v8::Uint32::New(src);
}

inline v8::Handle<v8::Value> to_v8(bool const src)
{
	return v8::Boolean::New(src);
}

} // namespace v8pp

#endif // V8PP_TO_V8_HPP_INCLUDED
