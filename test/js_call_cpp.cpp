#include "v8pp/call_from_v8.hpp"
#include "v8pp/context.hpp"
#include "v8pp/function.hpp"
#include "v8pp/ptr_traits.hpp"

#include "v8pp/call_v8.hpp"
#include "v8pp/module.hpp"
#include "v8pp/class.hpp"

#include "test.hpp"

#include "v8pp_utility.h"

#include <iostream>
using std::cout;
using std::endl;
using std::string;
#include <chrono>

#include "user_type.h"

static void elog(v8::FunctionCallbackInfo<v8::Value> const& args)
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

static void wlog(v8::FunctionCallbackInfo<v8::Value> const& args)
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

static void ilog(v8::FunctionCallbackInfo<v8::Value> const& args)
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

int64_t g_time_count = 0;
std::vector<std::pair<std::string, int64_t>> g_performance_results;
static void tbegin()
{
	g_time_count = std::chrono::system_clock::now().time_since_epoch().count();
}
static void tend(const std::string& op)
{
	int64_t time_count_now = std::chrono::system_clock::now().time_since_epoch().count();
	int64_t diff = time_count_now - g_time_count;
	g_performance_results.push_back({ op, diff });
	for (auto it : g_performance_results)
	{
		cout << "***** performance op:	" << it.first << "	diff:" << it.second << endl;
	}
}

#define P_JS_CALL_CPP_FUNC cout << "=========================================================== " << __func__ << " ====================" << endl;

int g_var;
int g_get_var()
{
	return g_var + 1;
}
void g_set_var(int x)
{
	g_var = x + 1;
}

// 这里可以看出，整个指针都是不支持js调过来的，js语言是没有指针的，所以这里没有暴，引用也不暴，只有const引用可以暴，切记！参数就是参数，这里禁用了参数作为返回的用法
static int test_param_int(int i /*, int* pi*/ /*, const int* cpi*/ /*, int& ri*/, const int& cri, uint32_t ui, const uint32_t& crui)
{
	P_JS_CALL_CPP_FUNC;
	cout << "i:" << i << endl;
	// cout << "pi:" << *pi << endl;
	//++*pi;
	// cout << "cpi:" << *cpi << endl;
	// cout << "ri:" << ri << endl;
	//++ri;
	cout << "cri:" << cri << endl;
	cout << "ui:" << ui << endl;
	cout << "crui:" << crui << endl;
	return 1048576;
}
static int64_t test_param_int64(int64_t i64, const int64_t& cri64, uint64_t ui64, const uint64_t& crui64)
{
	P_JS_CALL_CPP_FUNC;
	cout << "i64:" << i64 << endl;
	cout << "cri64:" << cri64 << endl;
	cout << "ui64:" << ui64 << endl;
	cout << "crui64:" << crui64 << endl;
	return (int64_t)4194967298;
}
// 与int同，所有可能修改参数作为返回的用法，全部禁用
// static const char* test_param_string(/*char* c, */ const char* cc, string s /*, string& rs*/, const string& crs /*, string* ps*/ /*, const string* cps*/)
// static const char* test_param_string(const char* cc, string s, const string& crs)
static const std::string& test_param_string(const char* cc, string s, const string& crs)
{
	P_JS_CALL_CPP_FUNC;
	// cout << "c:" << c << endl;
	//++c[0];
	cout << "cc:" << cc << endl;
	cout << "s:" << s << endl;
	// cout << "rs:" << rs << endl;
	//++rs[0];
	cout << "crs:" << crs << endl;
	// cout << "ps:" << *ps << endl;
	//++(*ps)[0];
	// cout << "cps:" << cps << endl;
	static std::string str = "hello_world_1024";
	// return str.c_str();
	return str;
}
// 注意用户自定义类型的指针引用const指针const引用都是ok的。用户自定义类型作为实例、引用、const引用返回都是不行的！！！只能作为指针、const指针返回能编译过，但是返回的是undefined.
// 所以统一返回int或bool，用指针或者引用作为参数返回需要的值，跟linux内核的c代码类似
static bool test_param_utype(UserTypeIS ut, UserTypeIS* put, const UserTypeIS* cput, UserTypeIS& rut, const UserTypeIS& crut)
// static UserTypeIS test_param_utype(UserTypeIS ut, UserTypeIS* put, const UserTypeIS* cput, UserTypeIS& rut, const UserTypeIS& crut)
// static const UserTypeIS& test_param_utype(UserTypeIS ut, UserTypeIS* put, const UserTypeIS* cput, UserTypeIS& rut, const UserTypeIS& crut)
// static UserTypeIS* test_param_utype(UserTypeIS ut, UserTypeIS* put, const UserTypeIS* cput, UserTypeIS& rut, const UserTypeIS& crut)
// static const UserTypeIS* test_param_utype(UserTypeIS ut, UserTypeIS* put, const UserTypeIS* cput, UserTypeIS& rut, const UserTypeIS& crut)
// static UserTypeIS& test_param_utype(UserTypeIS ut, UserTypeIS* put, const UserTypeIS* cput, UserTypeIS& rut, const UserTypeIS& crut)
{
	P_JS_CALL_CPP_FUNC;
	cout << "ut:" << ut << endl;
	cout << "put:" << *put << endl;
	cout << "cput:" << *cput << endl;
	put->nval *= 2;
	cout << "rut:" << rut << endl;
	rut.nval /= 2;
	cout << "crut:" << crut << endl;
	// static UserTypeIS sut(99999, "I_am_static");
	// return &sut;
	return true;
}

// 注意Array也是不能传指针不能传引用不能传const指针，只能传值或const引用!不允许函数内部修改
// static void test_param_arr_string(v8::Isolate* isolate, v8::Local<v8::Array> as /*, v8::Local<v8::Array>* pas*/ /*, v8::Local<v8::Array>& ras*/ /*, const v8::Local<v8::Array>* cpas*/, const v8::Local<v8::Array>& cras)
// static void test_param_arr_string(v8::Isolate* isolate, v8::Local<v8::Array> as, const v8::Local<v8::Array>& cras)
// static std::vector<std::string> test_param_arr_string(v8::Isolate* isolate, v8::Local<v8::Array> as, const v8::Local<v8::Array>& cras)
static std::list<std::string> test_param_arr_string(v8::Isolate* isolate, v8::Local<v8::Array> as, const v8::Local<v8::Array>& cras)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto strs = v8pp::from_v8<std::vector<std::string>>(isolate, as);
	cout << strs.size() << endl;
	std::for_each(strs.begin(), strs.end(), [](const std::string& s)
		{ cout << s << " "; });
	cout << endl;
	strs = v8pp::from_v8<std::vector<std::string>>(isolate, cras);
	cout << strs.size() << endl;
	std::for_each(strs.begin(), strs.end(), [](const std::string& s)
		{ cout << s << " "; });
	cout << endl;

	// 这样写会繁杂一点，但是省去了拷贝，性能会高一点
	if (!is_valid_v8array(isolate, as))
	{
		throw std::runtime_error("not Array");
	}
	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Array> array = as.As<v8::Array>();
	for (uint32_t i = 0, count = array->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> item = array->Get(context, i).ToLocalChecked();
		v8::Local<v8::String> v8str = item.As<v8::String>();
		auto pszStr = v8pp::convert<std::string>::from_v8(isolate, item);
		cout << pszStr << " ";
	}
	cout << endl;

	// 或者这样简单封一下
	loop_v8_array_of_val<string>(isolate, as, [](const std::string& str)
		{ cout << str << " "; return true; });
	cout << endl;
	// v8::EscapableHandleScope handle_scope(isolate);
	// auto ret = v8::Array::New(isolate, 3);
	// for (uint32_t i = 0, count = ret->Length(); i < count; ++i)
	//{
	//	v8::Local<v8::Value> val = v8pp::to_v8(isolate, 2 << i);
	//	ret->Set(context, i, val);
	// }
	// return handle_scope.Escape(ret);
	return { "hello", "world", "hello_world" };
}
static void performance_param_arr_str_v8(v8::Isolate* isolate, const v8::Local<v8::Array>& as, const v8::Local<v8::Array>& cras)
{
	P_JS_CALL_CPP_FUNC;

	loop_v8_array_of_val<string>(isolate, as, [](const std::string& str)
		{ cout << str << " "; return true; });
	cout << endl;

	loop_v8_array_of_val<string>(isolate, cras, [](const std::string& str)
		{ cout << str << " "; return true; });
	cout << endl;
}
// ***** performance op:performance_param_arr_str_v8	diff: 47278686
// *****performance op : performance_param_arr_str_cpp	diff: 29188187
// 可以看出，使用std::vector<std::string>会比原生的 v8::Local<v8::Array>的性能要高出不少，而且v8的用法还是封装过的原生的循环。
// !!! 所以，js调用c++的时候，统一封装c++的数据结构vector/list/map
// static void performance_param_arr_str_cpp(const std::vector<std::string>& as, const std::vector<std::string>& cras)
static void performance_param_arr_str_cpp(const std::list<std::string>& as, const std::list<std::string>& cras)
{
	P_JS_CALL_CPP_FUNC;
	std::for_each(as.begin(), as.end(), [](const std::string& s)
		{ cout << s << " "; });
	cout << endl;
	std::for_each(cras.begin(), cras.end(), [](const std::string& s)
		{ cout << s << " "; });
	cout << endl;
}
static void performance_param_map_is_v8(v8::Isolate* isolate, const v8::Local<v8::Map>& as, const v8::Local<v8::Map>& cras)
{
	P_JS_CALL_CPP_FUNC;
	loop_v8_map_of_val<int, std::string>(isolate, as, [](const int& k, const std::string& v)
		{ cout << k << "=>" << v << " "; return true; });
	cout << endl;
	loop_v8_map_of_val<int, std::string>(isolate, cras, [](const int& k, const std::string& v)
		{ cout << k << "=>" << v << " "; return true; });
	cout << endl;
}
// map的性能相当，cpp要略差一丢丢，但不大。好用的层面来说还是用stl方便
static void performance_param_map_is_cpp(const std::map<int, std::string>& as, const std::map<int, std::string>& cras)
// static void performance_param_map_is_cpp(const std::unordered_map<int, std::string>& as, const std::unordered_map<int, std::string>& cras)
{
	P_JS_CALL_CPP_FUNC;
	std::for_each(as.begin(), as.end(), [](const std::pair<int, std::string>& s)
		{ cout << s.first << "=>" << s.second << " "; return true; });
	cout << endl;
	std::for_each(cras.begin(), cras.end(), [](const std::pair<int, std::string>& s)
		{ cout << s.first << "=>" << s.second << " "; return true; });
	cout << endl;
}
// 其实特么就是 v8::Array ，不过处理是当int处理的，写成Int32Array更容易理解一点点
// static void test_param_arr_int(v8::Isolate* isolate, v8::Local<v8::Int32Array> ai /*, v8::Local<v8::Int32Array>* pai*/ /*, v8::Local<v8::Int32Array>& rai*/ /*, const v8::Local<v8::Int32Array>* cpai*/, const v8::Local<v8::Int32Array>& crai)
static void test_param_arr_int(v8::Isolate* isolate, v8::Local<v8::Int32Array> ai, const v8::Local<v8::Int32Array>& crai)
// static void test_param_arr_int(v8::Isolate* isolate, v8::Local<v8::Array> ai, const v8::Local<v8::Array>& crai)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto arr = v8pp::from_v8<std::vector<int>>(isolate, ai);
	cout << arr.size() << endl;
	std::for_each(arr.begin(), arr.end(), [](int i)
		{ cout << i << " "; });
	cout << endl;
	arr = v8pp::from_v8<std::vector<int>>(isolate, crai);
	cout << arr.size() << endl;
	std::for_each(arr.begin(), arr.end(), [](int i)
		{ cout << i << " "; });
	cout << endl;

	// 或者这样简单封一下
	loop_v8_array_of_val<int>(isolate, ai, [](const int& val)
		{ cout << val << " "; return true; });
	cout << endl;
}

static void test_param_arr_utype(v8::Isolate* isolate, v8::Local<v8::Array> au, const v8::Local<v8::Array>& crau)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto arr = v8pp::from_v8<std::vector<UserTypeIS>>(isolate, au);
	cout << arr.size() << endl;
	std::for_each(arr.begin(), arr.end(), [](const UserTypeIS& i)
		{ cout << i << " "; });
	cout << endl;
	arr = v8pp::from_v8<std::vector<UserTypeIS>>(isolate, crau);
	cout << arr.size() << endl;
	std::for_each(arr.begin(), arr.end(), [](const UserTypeIS& i)
		{ cout << i << " "; });
	cout << endl;

	// 或者这样简单封一下
	loop_v8_array_of_val<UserTypeIS>(isolate, au, [](const UserTypeIS& val)
		{ cout << val << " "; return true; });
	cout << endl;
}

// set not allowed, use Map<xxx, bool> instead. and set value to true.
// static void test_param_set_string(v8::Isolate* isolate, std::set<std::string> si, const std::set<std::string>& crsi)
static void test_param_set_string(v8::Isolate* isolate, v8::Local<v8::Value> si, const v8::Local<v8::Value>& crsi)
{
	P_JS_CALL_CPP_FUNC;
	if (si->IsSet())
	{
		cout << "set working" << endl;
		v8::Set* pset = v8::Set::Cast(*si);
		// auto set = v8pp::convert<std::set<std::string>>::from_v8(isolate, si);

		cout << pset->Size() << endl;
		for (uint32_t i = 0, count = pset->Size(); i < count; ++i)
		{
			v8::Local<v8::Value> item = pset->Get(isolate->GetCurrentContext(), i).ToLocalChecked();
			if (item->IsNullOrUndefined())
			{
				cout << "not string" << endl;
				continue;
			}
			auto val = v8pp::convert<std::string>::from_v8(isolate, item);
			cout << val << " " << endl;
		}
		cout << endl;
	}
	else
	{
		cout << "set not working" << endl;
	}
	// std::for_each(si.begin(), si.end(), [](const std::string& i)
	//	{ cout << i << " "; });
	// cout << endl;
	// std::for_each(crsi.begin(), crsi.end(), [](const std::string& i)
	//	{ cout << i << " "; });
	cout << endl;
}

// map/ unordered_map都是可以的，另外可以简单封装一下，或者是强遍历，会省去一次map/unordered_map的构造、拷贝、析构
static void test_param_map_int_int(v8::Isolate* isolate, v8::Local<v8::Map> ms, const v8::Local<v8::Map>& crms)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto melements = v8pp::from_v8<std::map<int, int>>(isolate, ms);
	cout << melements.size() << endl;
	std::for_each(melements.begin(), melements.end(), [](const std::pair<int, int>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;
	auto uelements = v8pp::from_v8<std::unordered_map<int, int>>(isolate, crms);
	cout << uelements.size() << endl;
	std::for_each(uelements.begin(), uelements.end(), [](const std::pair<int, int>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;

	// 这样写会繁杂一点，但是省去了拷贝，性能会高一点
	if (!is_valid(isolate, ms))
	{
		throw std::runtime_error("not map");
	}

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = ms.As<v8::Object>();
	v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();

	for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> key = prop_names->Get(context, i).ToLocalChecked();
		v8::Local<v8::Value> val = object->Get(context, key).ToLocalChecked();
		const auto k = v8pp::convert<int>::from_v8(isolate, key);
		const auto v = v8pp::convert<int>::from_v8(isolate, val);
		cout << k << "=>" << v << " ";
	}
	cout << endl;

	// 或者这样简单封一下
	loop_v8_map_of_val<int, int>(isolate, ms, [](const int& k, const int& v)
		{ cout << k << "=>" << v << " "; return true; });
	cout << endl;
}
static void test_param_map_int_string(v8::Isolate* isolate, v8::Local<v8::Map> m, const v8::Local<v8::Map>& crm)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto melements = v8pp::from_v8<std::map<int, std::string>>(isolate, m);
	cout << melements.size() << endl;
	std::for_each(melements.begin(), melements.end(), [](const std::pair<int, std::string>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;
	auto uelements = v8pp::from_v8<std::unordered_map<int, std::string>>(isolate, crm);
	cout << uelements.size() << endl;
	std::for_each(uelements.begin(), uelements.end(), [](const std::pair<int, std::string>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;

	// 这样写会繁杂一点，但是省去了拷贝，性能会高一点
	if (!is_valid(isolate, m))
	{
		throw std::runtime_error("not map");
	}

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = m.As<v8::Object>();
	v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();

	for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> key = prop_names->Get(context, i).ToLocalChecked();
		v8::Local<v8::Value> val = object->Get(context, key).ToLocalChecked();
		const auto k = v8pp::convert<int>::from_v8(isolate, key);
		const auto v = v8pp::convert<std::string>::from_v8(isolate, val);
		cout << k << "=>" << v << " ";
	}
	cout << endl;

	// 或者这样简单封一下
	loop_v8_map_of_val<int, std::string>(isolate, m, [](const int& k, const std::string& v)
		{ cout << k << "=>" << v << " "; return true; });
	cout << endl;
}
static void test_param_map_string_int(v8::Isolate* isolate, v8::Local<v8::Map> ms, const v8::Local<v8::Map>& crms)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto melements = v8pp::from_v8<std::map<std::string, int>>(isolate, ms);
	cout << melements.size() << endl;
	std::for_each(melements.begin(), melements.end(), [](const std::pair<std::string, int>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;
	auto uelements = v8pp::from_v8<std::unordered_map<std::string, int>>(isolate, crms);
	cout << uelements.size() << endl;
	std::for_each(uelements.begin(), uelements.end(), [](const std::pair<std::string, int>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;

	// 这样写会繁杂一点，但是省去了拷贝，性能会高一点
	if (!is_valid(isolate, ms))
	{
		throw std::runtime_error("not map");
	}

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = ms.As<v8::Object>();
	v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();

	for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> key = prop_names->Get(context, i).ToLocalChecked();
		v8::Local<v8::Value> val = object->Get(context, key).ToLocalChecked();
		const auto k = v8pp::convert<std::string>::from_v8(isolate, key);
		const auto v = v8pp::convert<int>::from_v8(isolate, val);
		cout << k << "=>" << v << " ";
	}
	cout << endl;

	// 或者这样简单封一下
	loop_v8_map_of_val<std::string, int>(isolate, ms, [](const std::string& k, const int& v)
		{ cout << k << "=>" << v << " "; return true; });
	cout << endl;
}
static void test_param_map_string_string(v8::Isolate* isolate, v8::Local<v8::Map> m, const v8::Local<v8::Map>& crm)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto melements = v8pp::from_v8<std::map<std::string, std::string>>(isolate, m);
	cout << melements.size() << endl;
	std::for_each(melements.begin(), melements.end(), [](const std::pair<std::string, std::string>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;
	auto uelements = v8pp::from_v8<std::unordered_map<std::string, std::string>>(isolate, crm);
	cout << uelements.size() << endl;
	std::for_each(uelements.begin(), uelements.end(), [](const std::pair<std::string, std::string>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;

	// 这样写会繁杂一点，但是省去了拷贝，性能会高一点
	if (!is_valid(isolate, m))
	{
		throw std::runtime_error("not map");
	}

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = m.As<v8::Object>();
	v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();

	for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> key = prop_names->Get(context, i).ToLocalChecked();
		v8::Local<v8::Value> val = object->Get(context, key).ToLocalChecked();
		const auto k = v8pp::convert<std::string>::from_v8(isolate, key);
		const auto v = v8pp::convert<std::string>::from_v8(isolate, val);
		cout << k << "=>" << v << " ";
	}
	cout << endl;

	// 或者这样简单封一下
	loop_v8_map_of_val<std::string, std::string>(isolate, m, [](const std::string& k, const std::string& v)
		{ cout << k << "=>" << v << " "; return true; });
	cout << endl;
}
static void test_param_map_int_utype(v8::Isolate* isolate, v8::Local<v8::Map> ms, const v8::Local<v8::Map>& crms)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto melements = v8pp::from_v8<std::map<int, UserTypeIS>>(isolate, ms);
	cout << melements.size() << endl;
	std::for_each(melements.begin(), melements.end(), [](const std::pair<int, UserTypeIS>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;
	auto uelements = v8pp::from_v8<std::unordered_map<int, UserTypeIS>>(isolate, crms);
	cout << uelements.size() << endl;
	std::for_each(uelements.begin(), uelements.end(), [](const std::pair<int, UserTypeIS>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;

	// 这样写会繁杂一点，但是省去了拷贝，性能会高一点
	if (!is_valid(isolate, ms))
	{
		throw std::runtime_error("not map");
	}

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = ms.As<v8::Object>();
	v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();

	for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> key = prop_names->Get(context, i).ToLocalChecked();
		v8::Local<v8::Value> val = object->Get(context, key).ToLocalChecked();
		const auto k = v8pp::convert<int>::from_v8(isolate, key);
		const auto v = v8pp::convert<UserTypeIS>::from_v8(isolate, val);
		cout << k << "=>" << v << " ";
	}
	cout << endl;

	// 或者这样简单封一下
	loop_v8_map_of_val<int, UserTypeIS>(isolate, ms, [](const int& k, const UserTypeIS& v)
		{ cout << k << "=>" << v << " "; return true; });
	cout << endl;
}
static void test_param_map_string_utype(v8::Isolate* isolate, v8::Local<v8::Map> ms, const v8::Local<v8::Map>& crms)
{
	P_JS_CALL_CPP_FUNC;
	// 这样写会多一次转换，多一次拷贝
	auto melements = v8pp::from_v8<std::map<std::string, UserTypeIS>>(isolate, ms);
	cout << melements.size() << endl;
	std::for_each(melements.begin(), melements.end(), [](const std::pair<std::string, UserTypeIS>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;
	auto uelements = v8pp::from_v8<std::unordered_map<std::string, UserTypeIS>>(isolate, crms);
	cout << uelements.size() << endl;
	std::for_each(uelements.begin(), uelements.end(), [](const std::pair<std::string, UserTypeIS>& pi)
		{ cout << pi.first << "=>" << pi.second << " "; });
	cout << endl;

	// 这样写会繁杂一点，但是省去了拷贝，性能会高一点
	if (!is_valid(isolate, ms))
	{
		throw std::runtime_error("not map");
	}

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = ms.As<v8::Object>();
	v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();

	for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> key = prop_names->Get(context, i).ToLocalChecked();
		v8::Local<v8::Value> val = object->Get(context, key).ToLocalChecked();
		const auto k = v8pp::convert<std::string>::from_v8(isolate, key);
		const auto v = v8pp::convert<UserTypeIS>::from_v8(isolate, val);
		cout << k << "=>" << v << " ";
	}
	cout << endl;

	// 或者这样简单封一下
	loop_v8_map_of_val<std::string, UserTypeIS>(isolate, ms, [](const std::string& k, const UserTypeIS& v)
		{ cout << k << "=>" << v << " "; return true; });
	cout << endl;
}

void js_call_cpp()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	context.function("dlog", dlog);
	context.function("ilog", ilog);
	context.function("wlog", wlog);
	context.function("elog", elog);
	context.function("tbegin", tbegin);
	context.function("tend", tend);

	//// bind free variables and functions
	// v8pp::module umod(isolate);
	// umod
	//	// set read-only attribute
	//	.const_("PI", 3.1415)
	//	// set variable available in JavaScript with name `var`
	//	.var("var", g_var)
	//	// set function get_var as `fun`
	//	.function("fun", &g_get_var)
	//	// set property `prop` with getter get_var() and setter set_var()
	//	.property("prop", g_get_var, g_set_var);

	reg_usertype(context);

	v8pp::class_<v8::Array> JsArray_Class(isolate);
	context.class_("Array", JsArray_Class);
	// v8pp::class_<v8::Set> JsSet_Class(isolate);
	// context.class_("Set", JsSet_Class);

	//// set bindings in global object as `umod`
	// context.module("umod", umod);

	// performance
	context.function("performance_param_arr_str_v8", performance_param_arr_str_v8);
	context.function("performance_param_arr_str_cpp", performance_param_arr_str_cpp);
	context.function("performance_param_map_is_v8", performance_param_map_is_v8);
	context.function("performance_param_map_is_cpp", performance_param_map_is_cpp);

	context.function("test_param_int", test_param_int);
	context.function("test_param_int64", test_param_int64);
	context.function("test_param_string", test_param_string);
	context.function("test_param_utype", test_param_utype);
	context.function("test_param_arr_string", test_param_arr_string);
	context.function("test_param_arr_int", test_param_arr_int);
	context.function("test_param_arr_utype", test_param_arr_utype);
	// set not allowed, use Map<xxx, bool> instead
	context.function("test_param_set_string", test_param_set_string);
	context.function("test_param_map_int_int", test_param_map_int_int);
	context.function("test_param_map_int_string", test_param_map_int_string);
	context.function("test_param_map_string_int", test_param_map_string_int);
	context.function("test_param_map_string_string", test_param_map_string_string);
	context.function("test_param_map_int_utype", test_param_map_int_utype);
	context.function("test_param_map_string_utype", test_param_map_string_utype);

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Function> fun = context.run_file("../../js/frame_update.js").As<v8::Function>();
	v8pp::call_v8(isolate, fun, fun);
	if (try_catch.HasCaught())
	{
		std::cout << "try_catch.HasCaught()" << std::endl;
		std::string const msg = v8pp::from_v8<std::string>(isolate,
			try_catch.Exception()->ToString(isolate->GetCurrentContext()).ToLocalChecked());
		throw std::runtime_error(msg);
	}
}
