//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_CONFIG_HPP_INCLUDED
#define V8PP_CONFIG_HPP_INCLUDED

/// v8::Isolate data slot number, used in v8pp for shared data
//#if !defined(V8PP_ISOLATE_DATA_SLOT)
//#define V8PP_ISOLATE_DATA_SLOT 0
//#endif

/// v8pp plugin initialization procedure name
#if !defined(V8PP_PLUGIN_INIT_PROC_NAME)
#define V8PP_PLUGIN_INIT_PROC_NAME v8pp_module_init
#endif

/// v8pp plugin filename suffix
#if !defined(V8PP_PLUGIN_SUFFIX)
	#if defined(WIN32)
	#define V8PP_PLUGIN_SUFFIX ".dll"
	#else
	#define V8PP_PLUGIN_SUFFIX ".so"
	#endif
#endif

#if defined(_MSC_VER)
	#define V8PP_EXPORT __declspec(dllexport)
	#define V8PP_IMPORT __declspec(dllimport)
#elif __GNUC__ >= 4
	#define V8PP_EXPORT __attribute__((__visibility__("default")))
	#define V8PP_IMPORT V8PP_EXPORT
#else
	#define V8PP_EXPORT
	#define V8PP_IMPORT
#endif

#define V8PP_PLUGIN_INIT(isolate) extern "C" V8PP_EXPORT \
v8::Local<v8::Value> V8PP_PLUGIN_INIT_PROC_NAME(isolate)

#ifndef V8PP_HEADER_ONLY
#define V8PP_HEADER_ONLY 1
#endif

#if V8PP_HEADER_ONLY
#define V8PP_IMPL inline
#else
#define V8PP_IMPL
#endif

#endif // V8PP_CONFIG_HPP_INCLUDED
