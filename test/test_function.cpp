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

	context.function("f", f);
	check_eq("f", run_script<int>(context, "f(1)"), 1);

	context.function("g", &g);
	check_eq("g", run_script<std::string>(context, "g('abc')"), "abc");

	context.function("h", h);
	check_eq("h", run_script<int>(context, "h(1, 2)"), 3);

	int x = 1, y = 2;
	struct moveonly
	{
		double v = 3;
		moveonly() = default;
		moveonly(moveonly const&) = delete;
		moveonly& operator=(moveonly const&) = delete;
		moveonly(moveonly&&) = default;
		moveonly& operator=(moveonly&&) = default;
	};
	moveonly z;
	context.function("lambda", [x, y, z = std::move(z)](int a) { return a + x + y + z.v; });
	check_eq("lambda", run_script<int>(context, "lambda(3)"), 9);

	auto lambda2 = []() { return 99; };
	//TODO: static_assert(v8pp::detail::external_data::is_bitcast_allowed<decltype(lambda2)>::value);

	context.function("lambda2", lambda2);
	check_eq("lambda2", run_script<int>(context, "lambda2()"), 99);

	X xfun;
	context.function("xfun", xfun);
	check_eq("xfun", run_script<int>(context, "xfun(5)"), -5);

	std::function<int(int)> fun = f;
	context.function("fun", fun);
	check_eq("fun", run_script<int>(context, "fun(42)"), 42);
}
