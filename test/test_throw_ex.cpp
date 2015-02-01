#include "v8pp/throw_ex.hpp"
#include "test.hpp"

namespace {

void test_(v8pp::context& context, std::string msg,
	v8::Local<v8::Value>(*exception_ctor)(v8::Handle<v8::String>))
{
	v8::Isolate* isolate = context.isolate();

	v8::HandleScope scope(isolate);

	v8::TryCatch try_catch;
	v8::Local<v8::Value> ex = v8pp::throw_ex(isolate, msg, exception_ctor);
	check(msg + " has caught", try_catch.HasCaught());

	//v8::String::Utf8Value err_msg(try_catch.Message()->Get());
	//check_eq("message", *err_msg, msg);

	try_catch.Reset();
}

} // unnamed namespace

void test_throw_ex()
{
	v8pp::context context;
	test_(context, "Error message", v8::Exception::Error);
	test_(context, "RangeError message", v8::Exception::RangeError);
	test_(context, "ReferenceError message", v8::Exception::ReferenceError);
	test_(context, "SyntaxError message", v8::Exception::SyntaxError);
	test_(context, "TypeError message", v8::Exception::TypeError);
}
