#ifndef V8PP_THROW_EX_HPP_INCLUDED
#define V8PP_THROW_EX_HPP_INCLUDED

#include <string_view>

#include <v8.h>

#include "v8pp/config.hpp"

namespace v8pp {

using exception_ctor = decltype(v8::Exception::Error); // assuming all Exception ctors have the same type
constexpr bool exception_ctor_with_options = V8_MAJOR_VERSION > 11 || (V8_MAJOR_VERSION == 11 && V8_MINOR_VERSION >= 9);

v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, std::string_view message,
	exception_ctor = {}, v8::Local<v8::Value> exception_options = {});

} // namespace v8pp

#if V8PP_HEADER_ONLY
#include "v8pp/throw_ex.ipp"
#endif

#endif // V8PP_THROW_EX_HPP_INCLUDED
