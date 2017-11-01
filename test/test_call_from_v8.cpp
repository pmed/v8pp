//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/call_from_v8.hpp"
#include "v8pp/context.hpp"
#include "v8pp/function.hpp"
#include "v8pp/ptr_traits.hpp"

#include "test.hpp"

namespace {

int x()
{
	return 0;
}
int y(int a)
{
	return a;
}
int z(v8::Isolate*, int a)
{
	return a;
}

void w(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return args.GetReturnValue().Set(args.Length());
}

struct X {};
void class_ref(X&) {}
void class_ptr(X*) {}
void class_sptr(std::shared_ptr<X>) {}

using v8pp::detail::select_call_traits;
using v8pp::detail::call_from_v8_traits;
using v8pp::detail::isolate_arg_call_traits;
using v8pp::detail::v8_args_call_traits;

using v8pp::raw_ptr_traits;
using v8pp::shared_ptr_traits;

static_assert(select_call_traits<decltype(&x)>::arg_count == 0, "x argc count");
static_assert(std::is_same<select_call_traits<decltype(&x)>::arg_type<0>,
	void>::value, "x has no args");

static_assert(select_call_traits<decltype(&y)>::arg_count == 1, "y argc count");
static_assert(std::is_same<select_call_traits<decltype(&y)>::arg_type<0>,
	int>::value, "y 1st arg");

static_assert(select_call_traits<decltype(&z)>::arg_count == 1, "z argc count");
static_assert(std::is_same<select_call_traits<decltype(&z)>::arg_type<0>,
	v8::Isolate*>::value, "z 1st arg");
static_assert(std::is_same<select_call_traits<decltype(&z)>::arg_type<1>,
	int>::value, "z 2nd arg");

static_assert(std::is_same<select_call_traits<decltype(&x)>,
	call_from_v8_traits<decltype(&x)>> ::value, "");

static_assert(std::is_same<select_call_traits<decltype(&z)>,
	isolate_arg_call_traits<decltype(&z)>> ::value, "");

static_assert(std::is_same<select_call_traits<decltype(&w)>,
	v8_args_call_traits<decltype(&w)>> ::value, "");

static_assert(std::is_same<select_call_traits<decltype(&class_ptr)>::arg_convert<0, raw_ptr_traits>::from_type, X*>::value, "class ptr");
static_assert(std::is_same<select_call_traits<decltype(&class_ref)>::arg_convert<0, raw_ptr_traits>::from_type, X&>::value, "class ref");
static_assert(std::is_same<select_call_traits<decltype(&class_sptr)>::arg_convert<0, raw_ptr_traits>::from_type, std::shared_ptr<X>>::value, "class shared_ptr");

static_assert(std::is_same<select_call_traits<decltype(&class_ptr)>::arg_convert<0, shared_ptr_traits>::from_type, std::shared_ptr<X>>::value, "class ptr");
static_assert(std::is_same<select_call_traits<decltype(&class_ref)>::arg_convert<0, shared_ptr_traits>::from_type, X&>::value, "class ref");
static_assert(std::is_same<select_call_traits<decltype(&class_sptr)>::arg_convert<0, shared_ptr_traits>::from_type, std::shared_ptr<X>>::value, "class shared_ptr");

} // unnamed namespace

void test_call_from_v8()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	context.function("x", x);
	context.function("y", y);
	context.function("z", z);
	context.function("w", w);

	check_eq("x", run_script<int>(context, "x()"), 0);
	check_eq("y", run_script<int>(context, "y(1)"), 1);
	check_eq("z", run_script<int>(context, "z(2)"), 2);
	check_eq("w", run_script<int>(context, "w(2, 'd', true, null)"), 4);
}
