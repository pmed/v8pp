#ifndef V8PP_JSON_HPP_INCLUDED
#define V8PP_JSON_HPP_INCLUDED

#include <string>

#include <v8.h>

#include "v8pp/config.hpp"
#include "v8pp/utility.hpp"

namespace v8pp {

/// Stringify V8 value to JSON
/// return empty string for empty value
std::string json_str(v8::Isolate* isolate, v8::Local<v8::Value> value);

/// Parse JSON string into V8 value
/// return empty value for empty string
/// return Error value on parse error
v8::Local<v8::Value> json_parse(v8::Isolate* isolate, string_view str);

/// Convert wrapped C++ object to JavaScript object with properties
/// and optionally functions set from the C++ object
v8::Local<v8::Object> json_object(v8::Isolate* isolate, v8::Local<v8::Object> object,
	bool with_functions = false);

} // namespace v8pp

#if V8PP_HEADER_ONLY
#include "v8pp/json.ipp"
#endif

#endif // V8PP_JSON_HPP_INCLUDED
