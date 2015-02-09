#include "v8pp/call_from_v8.hpp"
#include "v8pp/context.hpp"
#include "v8pp/function.hpp"

#include "test.hpp"

static int x() { return 0; }
static int y(int a) { return a; }
static int z(v8::Isolate*, int a) { return a; }

static void w(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return args.GetReturnValue().Set(args.Length());
}

static_assert(v8pp::detail::select_call_traits<decltype(&x)>::arg_count == 0, "x argc count");
static_assert(std::is_same<v8pp::detail::select_call_traits<decltype(&x)>::arg_type<0>,
	void>::value, "x has no args");

static_assert(v8pp::detail::select_call_traits<decltype(&y)>::arg_count == 1, "y argc count");
static_assert(std::is_same<v8pp::detail::select_call_traits<decltype(&y)>::arg_type<0>,
	int>::value, "y 1st arg");

static_assert(v8pp::detail::select_call_traits<decltype(&z)>::arg_count == 1, "z argc count");
static_assert(std::is_same<v8pp::detail::select_call_traits<decltype(&z)>::arg_type<0>,
	v8::Isolate*>::value, "z 1st arg");
static_assert(std::is_same<v8pp::detail::select_call_traits<decltype(&z)>::arg_type<1>,
	int>::value, "z 2nd arg");

static_assert(std::is_same<v8pp::detail::select_call_traits<decltype(&x)>,
	v8pp::detail::call_from_v8_traits<decltype(&x) >> ::value, "");

static_assert(std::is_same<v8pp::detail::select_call_traits<decltype(&z)>,
	v8pp::detail::isolate_arg_call_traits<decltype(&z) >> ::value, "");

static_assert(std::is_same<v8pp::detail::select_call_traits<decltype(&w)>,
	v8pp::detail::v8_args_call_traits<decltype(&w) >> ::value, "");

void test_call_from_v8()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	context.set("x", v8pp::wrap_function(isolate, "x", &x));
	context.set("y", v8pp::wrap_function(isolate, "y", &y));
	context.set("z", v8pp::wrap_function(isolate, "z", &z));
	context.set("w", v8pp::wrap_function(isolate, "w", &w));

	check_eq("x", run_script<int>(context, "x()"), 0);
	check_eq("y", run_script<int>(context, "y(1)"), 1);
	check_eq("z", run_script<int>(context, "z(2)"), 2);
	check_eq("w", run_script<int>(context, "w(2, 'd', true, null)"), 4);
}
