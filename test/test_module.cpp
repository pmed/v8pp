#include "v8pp/module.hpp"
#include "v8pp/context.hpp"
#include "v8pp/property.hpp"

#include "test.hpp"

#include <type_traits>

static std::string var;

static int fun(int x) { return x + 1; }

static int x = 1;
static int get_x() { return x + 1; }
static void set_x(int v) { x = v - 1; }

static_assert(std::is_move_constructible<v8pp::module>::value, "");
static_assert(std::is_move_assignable<v8pp::module>::value, "");
static_assert(!std::is_copy_assignable<v8pp::module>::value, "");
static_assert(!std::is_copy_constructible<v8pp::module>::value, "");

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
		.set("consts", consts)
		.set("var", var)
		.set("fun", &fun)
		.set("empty", v8::Null(context.isolate()))
		.set("rprop", v8pp::property(get_x))
		.set("wprop", v8pp::property(get_x, set_x))
		;

	context.set("module", module);

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
