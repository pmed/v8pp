//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/module.hpp"
#include "v8pp/context.hpp"
#include "v8pp/property.hpp"

#include "test.hpp"

static std::string var;

static int fun(int x) { return x + 1; }

static int x = 1;
static int get_x() { return x + 1; }
static void set_x(int v) { x = v - 1; }

void test_module()
{
	v8pp::context context;

	v8::HandleScope scope(context.isolate());

	v8pp::module module(context.isolate());
	v8pp::module consts(context.isolate());

	consts
		.set_const("bool", true)
		.set_const("char", 'Z')
		.set_const("int", 100)
		.set_const("str", "str")
		.set_const("num", 99.9)
		;

	module
		.set_submodule("consts", consts)
		.set_var("var", var)
		.set_function("fun", &fun)
		.set_value("empty", v8::Null(context.isolate()))
		.set_property("rprop", get_x)
		.set_property("wprop", get_x, set_x)
		;
	context.set_module("module", module);

	check_eq("module.consts.bool",
		run_script<bool>(context, "module.consts.bool"), true);
	check_eq("module.consts.char",
		run_script<char>(context, "module.consts.char"), 'Z');
	check_eq("module.consts.int",
		run_script<char>(context, "module.consts.int"), 100);
	check_eq("module.consts.str",
		run_script<std::string>(context, "module.consts.str"), "str");

	check_eq("module.var", run_script<std::string>(context,
		"module.var = 'test'; module.var"), "test");
	check_eq("var", var, "test");

	check_eq("module.fun",
		run_script<int>(context, "module.fun(100)"), 101);

	check_eq("module.rprop",
		run_script<int>(context, "module.rprop"), 2);
	check_eq("module.wrop",
		run_script<int>(context, "++module.wprop"), 3);
	check_eq("x", x, 2);
}
