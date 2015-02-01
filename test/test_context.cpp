#include "v8pp/context.hpp"

#include "test.hpp"

void test_context()
{
	v8pp::context context;

	v8::HandleScope scope(context.isolate());
	int const r = context.run_script("42")->Int32Value();
	check_eq("run_script", r, 42);
}
