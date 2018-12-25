//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_OBJECT_HPP_INCLUDED
#define V8PP_OBJECT_HPP_INCLUDED

#include <cstring>

#include <v8.h>

#include "v8pp/convert.hpp"

namespace v8pp {

/// Get optional value from V8 object by name.
/// Dot symbols in option name delimits subobjects name.
/// return false if the value doesn't exist in the options object
template<typename T>
bool get_option(v8::Isolate* isolate, v8::Local<v8::Object> options,
	char const* name, T& value)
{
	char const* dot = strchr(name, '.');
	if (dot)
	{
		std::string const subname(name, dot);
		v8::Local<v8::Object> suboptions;
		return get_option(isolate, options, subname.c_str(), suboptions)
			&& get_option(isolate, suboptions, dot + 1, value);
	}
	v8::Local<v8::Value> val;
	if (!options->Get(isolate->GetCurrentContext(), v8pp::to_v8(isolate, name)).ToLocal(&val)
		|| val->IsUndefined())
	{
		return false;
	}
	value = from_v8<T>(isolate, val);
	return true;
}

/// Set named value in V8 object
/// Dot symbols in option name delimits subobjects name.
/// return false if the value doesn't exists in the options subobject
template<typename T>
bool set_option(v8::Isolate* isolate, v8::Local<v8::Object> options,
	char const* name, T const& value)
{
	char const* dot = strchr(name, '.');
	if (dot)
	{
		std::string const subname(name, dot);
		v8::HandleScope scope(isolate);
		v8::Local<v8::Object> suboptions;
		return get_option(isolate, options, subname.c_str(), suboptions)
			&& set_option(isolate, suboptions, dot + 1, value);
	}
	return options->Set(isolate->GetCurrentContext(), v8pp::to_v8(isolate, name), to_v8(isolate, value)).FromJust();
}

/// Set named constant in V8 object
/// Subobject names are not supported
template<typename T>
void set_const(v8::Isolate* isolate, v8::Local<v8::Object> options,
	char const* name, T const& value)
{
	options->DefineOwnProperty(isolate->GetCurrentContext(),
		v8pp::to_v8(isolate, name), to_v8(isolate, value),
		v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete)).FromJust();
}

} // namespace v8pp

#endif // V8PP_OBJECT_HPP_INCLUDED
