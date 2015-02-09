#include "v8pp/function.hpp"
#include "v8pp/context.hpp"

#include "test.hpp"

static int f(int const& x) { return x; }
static std::string g(char const* s) { return s? s : ""; }
static int h(v8::Isolate*, int x, int y) { return x + y; }

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
}
