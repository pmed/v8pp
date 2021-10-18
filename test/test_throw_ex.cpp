#include "v8pp/throw_ex.hpp"
#include "test.hpp"

namespace {

void test(v8pp::context& context, std::string const& type,
	v8::Local<v8::Value> (*exception_ctor)(v8::Local<v8::String>))
{
	v8::Isolate* isolate = context.isolate();

	v8::HandleScope scope(isolate);

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Value> ex = v8pp::throw_ex(isolate,
		"exception message", exception_ctor);
	check(" has caught", try_catch.HasCaught());
	check("the same stack trace", try_catch.Message()->GetStackTrace() == v8::Exception::GetStackTrace(ex));
	v8::String::Utf8Value const err_msg(isolate, try_catch.Message()->Get());
	check_eq("message", *err_msg, "Uncaught " + type + ": exception message");
}

} // unnamed namespace

void test_throw_ex()
{
	v8pp::context context;
	test(context, "Error", v8::Exception::Error);
	test(context, "RangeError", v8::Exception::RangeError);
	test(context, "ReferenceError", v8::Exception::ReferenceError);
	test(context, "SyntaxError", v8::Exception::SyntaxError);
	test(context, "TypeError", v8::Exception::TypeError);
}
