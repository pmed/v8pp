#include "v8pp/throw_ex.hpp"
#include "v8pp/json.hpp"
#include "test.hpp"

namespace {

void test(v8pp::context& context, std::string type, v8pp::exception_ctor ctor, v8::Local<v8::Value> opts = {})
{
	v8::Isolate* isolate = context.isolate();

	v8::HandleScope scope(isolate);

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Value> ex = v8pp::throw_ex(isolate, "exception message", ctor, opts);
	check(" has caught", try_catch.HasCaught());
	check("the same stack trace", try_catch.Message()->GetStackTrace() == v8::Exception::GetStackTrace(ex));
	v8::String::Utf8Value const err_msg(isolate, try_catch.Message()->Get());
	check_eq("message", *err_msg, "Uncaught " + type + ": exception message");
}

} // unnamed namespace

void test_throw_ex()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();

	test(context, "Error", v8::Exception::Error);
	test(context, "RangeError", v8::Exception::RangeError);
	test(context, "ReferenceError", v8::Exception::ReferenceError);
	test(context, "SyntaxError", v8::Exception::SyntaxError);
	test(context, "TypeError", v8::Exception::TypeError);

	if constexpr (v8pp::exception_ctor_with_options)
	{
		v8::HandleScope scope(isolate);
		test(context, "Error", v8::Exception::Error, v8pp::json_parse(isolate, "{ 'opt1': true, 'opt2': 'str' }"));
	}
}
