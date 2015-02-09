#include "v8pp/call_v8.hpp"
#include "v8pp/context.hpp"

#include "test.hpp"

static void v8_arg_count(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(args.Length());
}

void test_call_v8()
{
	v8pp::context context;

	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);
	v8::Handle<v8::Function> fun = v8::Function::New(isolate, v8_arg_count);

	check_eq("no args", v8pp::call_v8(isolate, fun, fun)->Int32Value(), 0);
	check_eq("1 arg", v8pp::call_v8(isolate, fun, fun, 1)->Int32Value(), 1);
	check_eq("2 args", v8pp::call_v8(isolate, fun, fun, true, 2.2)->Int32Value(), 2);
	check_eq("3 args", v8pp::call_v8(isolate, fun, fun, 1, true, "abc")->Int32Value(), 3);
}
