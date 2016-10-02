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

namespace v8pp {

/// Stringify V8 value to JSON
/// return empty string for empty value
inline std::string json_str(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
	if (value.IsEmpty())
	{
		return std::string();
	}

	v8::HandleScope scope(isolate);

	v8::Local<v8::Object> json = isolate->GetCurrentContext()->
		Global()->Get(v8::String::NewFromUtf8(isolate, "JSON"))->ToObject();
	v8::Local<v8::Function> stringify = json->Get(v8::String::NewFromUtf8(isolate, "stringify")).As<v8::Function>();

	v8::Local<v8::Value> result = stringify->Call(json, 1, &value);
	v8::String::Utf8Value const str(result);

	return std::string(*str, str.length());
}

/// Parse JSON string into V8 value
/// return empty value for empty string
/// return Error value on parse error
inline v8::Handle<v8::Value> json_parse(v8::Isolate* isolate, std::string const& str)
{
	if (str.empty())
	{
		return v8::Handle<v8::Value>();
	}

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> json = isolate->GetCurrentContext()->
		Global()->Get(v8::String::NewFromUtf8(isolate, "JSON"))->ToObject();
	v8::Local<v8::Function> parse = json->Get(v8::String::NewFromUtf8(isolate, "parse")).As<v8::Function>();

	v8::Local<v8::Value> value = v8::String::NewFromUtf8(isolate, str.data(),
		v8::String::kNormalString, static_cast<int>(str.size()));

	v8::TryCatch try_catch;
	v8::Local<v8::Value> result = parse->Call(json, 1, &value);
	if (try_catch.HasCaught())
	{
		result = try_catch.Exception();
	}
	return scope.Escape(result);
}

} // namespace v8pp

#endif // V8PP_JSON_HPP_INCLUDED
