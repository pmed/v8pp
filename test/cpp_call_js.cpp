#include "v8pp/call_v8.hpp"
#include "v8pp/context.hpp"

#include "test.hpp"
#include <iostream>
using std::cout;
using std::endl;

#include "user_type.h"

static void dlog(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::HandleScope scope(args.GetIsolate());

	for (int i = 0; i < args.Length(); ++i)
	{
		if (i > 0) cout << ' ';
		v8::String::Utf8Value const str(args.GetIsolate(), args[i]);
		cout << *str;
	}
	cout << endl;
}

v8::Local<v8::Function> fun_cpp_call_js;
v8::Local<v8::Function> fun_addNumbers;

v8::Local<v8::Function> js_param_int;
v8::Local<v8::Function> js_param_string;
v8::Local<v8::Function> js_param_utype;
void test_functions(v8pp::context& context, v8::Isolate* isolate)
{
	v8::Local<v8::Value> recv;
	int64_t i64 = 0;
	uint64_t u64 = 0;
	std::string sret;
	int iret = 0;

	js_param_int = context.run_script("js_param_int").As<v8::Function>();
	js_param_string = context.run_script("js_param_string").As<v8::Function>();
	js_param_utype = context.run_script("js_param_utype").As<v8::Function>();

	// ######################################################## 内置类型
	// 看下来所有内置类型(int/uint/int64/uint64/string）都只支持实例、引用、const引用，不支持指针，不支持const指针。char*和const char*是可以的
	u64 = 4294967296;
	const uint64_t& cru64 = u64;
	uint64_t& ru64 = u64;
	uint64_t* pu64 = &u64;
	const uint64_t* cpu64 = &u64;
	// int/int64/uint64之类的可传实例，可传引用，可传const引用，不能传指针，不能传const指针
	recv = v8pp::call_v8(isolate, js_param_int, js_param_int, 1024, 2147483648 /*, pu64*/ /*, cpu64*/, ru64, cru64);
	i64 = v8pp::convert<int64_t>::from_v8(isolate, recv);
	cout << "i64:" << i64 << endl;

	std::string s = "asdf ";
	const std::string& crs = s;
	std::string& rs = s;
	const std::string* cps = &s;
	std::string* ps = &s;
	// string类型可传const char*，可传char*，可传string实例，可传string&， 可传const string&，不能传指针，不能传const指针
	recv = v8pp::call_v8(isolate, js_param_string, js_param_string, (char*)"hello ", std::string("world "), rs, crs);
	// recv = v8pp::call_v8(isolate, js_param_string, js_param_string, (const char*)"hello ", std::string("world "), ps, cps);
	sret = v8pp::convert<std::string>::from_v8(isolate, recv);
	cout << "sret:" << sret << endl;

	// ######################################################## usertype
	// usertype一直报错，暂时只找到用tuple传多个参数的方式，很不方便，返回也一直不行，tuple返回倒是可以，而且可以嵌套
	UserTypeIS ut1(111, "hello");
	UserTypeIS ut(222, "world");
	UserTypeIS* put = &ut;
	const UserTypeIS* cput = &ut;
	UserTypeIS& rut = ut;
	const UserTypeIS& crut = ut;
	recv = v8pp::call_v8(isolate, js_param_utype, js_param_utype, std::tuple<int, string>(111, "hello") /*, ut1*/ /*, put, cput, rut*/ /*, crut*/);
	// recv = v8pp::call_v8(isolate, js_param_string, js_param_string, (const char*)"hello ", std::string("world "), ps, cps);
	// iret = v8pp::convert<int>::from_v8(isolate, recv);
	// cout << "iret:" << iret << endl;
	std::tuple<int, std::tuple<int, string>> ret_tuple = v8pp::convert<std::tuple<int, std::tuple<int, string>>>::from_v8(isolate, recv);
	cout << "ret_tuple:" << ret_tuple << endl;
}

void cpp_call_js()
{
	v8pp::context context;

	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	context.function("dlog", dlog);
	context.function("include", [&context](const std::string& file)
		{ context.run_file(file); });

	reg_usertype(context);

	v8::TryCatch try_catch(isolate);
	// context.run_file("../../js/import_test.js");
	// v8::Local<v8::Function> fun_cpp_call_js = context.run_file("../../js/cpp_call_js.js").As<v8::Function>();

	// 1. run all files, or one entry file.
	context.run_file("../../js/cpp_call_js.js");

	// 2. get all functions and save to global, for other usage.
	fun_cpp_call_js = context.run_script("cpp_call_js_func").As<v8::Function>();
	fun_addNumbers = context.run_script("addNumbers").As<v8::Function>();

	// 3. just call the js function.
	v8pp::call_v8(isolate, fun_cpp_call_js, fun_cpp_call_js);

	v8::Local<v8::Value> recv;
	int add_result = 0;

	// 3.1 and we can also get the return value.
	recv = v8pp::call_v8(isolate, fun_addNumbers, fun_addNumbers, 111, 222);
	add_result = v8pp::convert<int>::from_v8(isolate, recv);
	cout << "add_result:" << add_result << endl;

	v8pp::call_v8(isolate, fun_cpp_call_js, fun_cpp_call_js);

	recv = v8pp::call_v8(isolate, fun_addNumbers, fun_addNumbers, 222, 333);
	add_result = v8pp::convert<int>::from_v8(isolate, recv);
	cout << "add_result:" << add_result << endl;

	v8pp::call_v8(isolate, fun_cpp_call_js, fun_cpp_call_js);

	test_functions(context, isolate);

	if (try_catch.HasCaught())
	{
		std::cout << "try_catch.HasCaught()" << std::endl;
		std::string const msg = v8pp::from_v8<std::string>(isolate,
			try_catch.Exception()->ToString(isolate->GetCurrentContext()).ToLocalChecked());
		throw std::runtime_error(msg);
	}
}
