#pragma once

#include <string_view>

#include <v8.h>

#include "v8pp/config.hpp"

namespace v8pp {

using exception_ctor = decltype(v8::Exception::Error); // assuming all Exception ctors have the same type
constexpr bool exception_ctor_with_options = V8_MAJOR_VERSION > 11 || (V8_MAJOR_VERSION == 11 && V8_MINOR_VERSION >= 9);

v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, std::string_view message,
	exception_ctor ctor = v8::Exception::Error, v8::Local<v8::Value> exception_options = {});

v8::Local<v8::Value> throw_error(v8::Isolate* isolate, std::string_view message,
	v8::Local<v8::Value> exception_options = {});

v8::Local<v8::Value> throw_range_error(v8::Isolate* isolate, std::string_view message,
	v8::Local<v8::Value> exception_options = {});

v8::Local<v8::Value> throw_reference_error(v8::Isolate* isolate, std::string_view message,
	v8::Local<v8::Value> exception_options = {});

v8::Local<v8::Value> throw_syntax_error(v8::Isolate* isolate, std::string_view message,
	v8::Local<v8::Value> exception_options = {});

v8::Local<v8::Value> throw_type_error(v8::Isolate* isolate, std::string_view message,
	v8::Local<v8::Value> exception_options = {});

} // namespace v8pp

#if V8PP_HEADER_ONLY
#include "v8pp/throw_ex.ipp"
#endif
