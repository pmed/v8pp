#include "v8pp/function.hpp"
#include "v8pp/context.hpp"

#include "test.hpp"

static int f(int const& x) { return x; }
static std::string g(char const* s) { return s ? s : ""; }
static int h(v8::Isolate*, int x, int y) { return x + y; }

struct X
{
	int operator()(int x) const { return -x; }
};

void test_function()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	context.set("f", v8pp::wrap_function(isolate, "f", &f));
	check_eq("f", run_script<int>(context, "f(1)"), 1);

	context.set("g", v8pp::wrap_function(isolate, "g", &g));
	check_eq("g", run_script<std::string>(context, "g('abc')"), "abc");

	context.set("h", v8pp::wrap_function(isolate, "h", &h));
	check_eq("h", run_script<int>(context, "h(1, 2)"), 3);

	int x = 1, y = 2;
	auto lambda = [x, y](int z) { return x + y + z; };
	context.set("lambda", v8pp::wrap_function(isolate, "lambda", lambda));
	check_eq("lambda", run_script<int>(context, "lambda(3)"), 6);

	auto lambda2 = []() { return 99; };
	context.set("lambda2", v8pp::wrap_function(isolate, "lambda2", lambda2));
	check_eq("lambda2", run_script<int>(context, "lambda2()"), 99);

	X xfun;
	context.set("xfun", v8pp::wrap_function(isolate, "xfun", xfun));
	check_eq("xfun", run_script<int>(context, "xfun(5)"), -5);

	std::function<int(int)> fun = f;
	context.set("fun", v8pp::wrap_function(isolate, "fun", fun));
	check_eq("fun", run_script<int>(context, "fun(42)"), 42);
}
