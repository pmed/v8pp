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
#include <vector>

#include <v8.h>

#ifdef WIN32
#include <windows.h>
#endif

namespace v8pp {

inline v8::Handle<v8::Value> throw_ex(v8::Isolate* isolate, char const* str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Handle<v8::String>) = v8::Exception::Error)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Handle<v8::String> message;
#ifdef _WIN32
	int const len = ::MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	if (len > 0)
	{
		std::vector<wchar_t> buf(len);
		::MultiByteToWideChar(CP_ACP, 0, str, -1, &buf[0], len);
		uint16_t const* data = reinterpret_cast<uint16_t const*>(&buf[0]);
		message = v8::String::NewFromTwoByte(isolate, data, v8::String::kNormalString, len - 1);
	}
#else
	message = v8::String::NewFromUtf8(isolate, str);
#endif
	return scope.Escape(isolate->ThrowException(exception_ctor(message)));
}

inline v8::Handle<v8::Value> throw_ex(v8::Isolate* isolate, std::string const& str,
	v8::Local<v8::Value> (*exception_ctor)(v8::Handle<v8::String>) = v8::Exception::Error)
{
	return throw_ex(isolate, str.c_str(), exception_ctor);
}

} // namespace v8pp

#endif // V8PP_THROW_EX_HPP_INCLUDED
