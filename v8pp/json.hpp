//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_JSON_HPP_INCLUDED
#define V8PP_JSON_HPP_INCLUDED

#include <string>

#include <v8.h>

#include "v8pp/config.hpp"

namespace v8pp {

/// Stringify V8 value to JSON
/// return empty string for empty value
std::string json_str(v8::Isolate* isolate, v8::Local<v8::Value> value);

/// Parse JSON string into V8 value
/// return empty value for empty string
/// return Error value on parse error
v8::Local<v8::Value> json_parse(v8::Isolate* isolate, std::string const& str);

/// Convert wrapped C++ object to JavaScript object with properties
/// and optionally functions set from the C++ object
v8::Local<v8::Object> json_object(v8::Isolate* isolate, v8::Local<v8::Object> object,
	bool with_functions = false);

} // namespace v8pp

#if V8PP_HEADER_ONLY
#include "v8pp/json.ipp"
#endif

#endif // V8PP_JSON_HPP_INCLUDED
