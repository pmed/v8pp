#include "v8pp/json.hpp"
#include "v8pp/context.hpp"
#include "v8pp/object.hpp"

#include "test.hpp"

void test_json()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8::Local<v8::Value> v;
	std::string str;

	str = v8pp::json_str(isolate, v);
	v = v8pp::json_parse(isolate, str);
	check("empty string", str.empty());
	check("empty parse", v.IsEmpty());

	v = v8::Integer::New(isolate, 42);

	str = v8pp::json_str(isolate, v);
	v = v8pp::json_parse(isolate, "42");
	check_eq("int string", str, "42");
	check_eq("int parse", v->Int32Value(isolate->GetCurrentContext()).FromJust(), 42);

	v8::Local<v8::Object> obj = v8::Object::New(isolate);
	v8pp::set_option(isolate, obj, "x", 1);
	v8pp::set_option(isolate, obj, "y", 2.2);
	v8pp::set_option(isolate, obj, "z", "abc");

	str = v8pp::json_str(isolate, obj);
	v = v8pp::json_parse(isolate, str);
	check_eq("object string", str, R"({"x":1,"y":2.2,"z":"abc"})");
	check_eq("object parse", v8pp::json_str(isolate, v), str);

	v = v8pp::json_object(isolate, v.As<v8::Object>());
	check_eq("json object", v8pp::json_str(isolate, v), str);

	v8::Local<v8::Array> arr = v8::Array::New(isolate, 1);
	arr->Set(isolate->GetCurrentContext(), 0, obj).FromJust();

	str = v8pp::json_str(isolate, arr);
	v = v8pp::json_parse(isolate, str);
	check_eq("array string", str, R"([{"x":1,"y":2.2,"z":"abc"}])");
	check_eq("array parse", v8pp::json_str(isolate, v), str);

	v = v8pp::json_parse(isolate, "blah-blah");
	check("parse error", v->IsNativeError());
}
