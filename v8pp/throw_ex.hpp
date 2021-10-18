#ifndef V8PP_THROW_EX_HPP_INCLUDED
#define V8PP_THROW_EX_HPP_INCLUDED

#include <string>

#include <v8.h>

#include "v8pp/config.hpp"
#include "v8pp/utility.hpp"

namespace v8pp {

v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, string_view str);

v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, string_view str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>));

} // namespace v8pp

#if V8PP_HEADER_ONLY
#include "v8pp/throw_ex.ipp"
#endif

#endif // V8PP_THROW_EX_HPP_INCLUDED
