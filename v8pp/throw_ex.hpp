//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_THROW_EX_HPP_INCLUDED
#define V8PP_THROW_EX_HPP_INCLUDED

#include <string>

#include <v8.h>

#include "v8pp/config.hpp"

namespace v8pp {

v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, char const* str);

v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, char const* str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>));

inline v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, std::string const& str)
{
	return throw_ex(isolate, str.c_str());
}

inline v8::Local<v8::Value> throw_ex(v8::Isolate* isolate, std::string const& str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>))
{
	return throw_ex(isolate, str.c_str(), exception_ctor);
}

} // namespace v8pp

#if V8PP_HEADER_ONLY
#include "v8pp/throw_ex.ipp"
#endif

#endif // V8PP_THROW_EX_HPP_INCLUDED
