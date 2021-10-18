#include "v8pp/call_from_v8.hpp"
#include "v8pp/context.hpp"
#include "v8pp/function.hpp"
#include "v8pp/ptr_traits.hpp"

#include "test.hpp"

static int x()
{
	return 0;
}
static int y(int a)
{
	return a;
}
static int z(v8::Isolate*, int a)
{
	return a;
}

static void w(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return args.GetReturnValue().Set(args.Length());
}

struct X;
void class_ref(X&);
void class_ptr(X*);
void class_sptr(std::shared_ptr<X>);

using v8pp::detail::select_call_traits;
using v8pp::detail::call_from_v8_traits;
using v8pp::detail::isolate_arg_call_traits;
using v8pp::detail::v8_args_call_traits;

using v8pp::raw_ptr_traits;
using v8pp::shared_ptr_traits;

static_assert(select_call_traits<decltype(&x)>::arg_count == 0, "x argc count");
static_assert(std::is_same<select_call_traits<decltype(&x)>::arg_type<0>, void>::value, "x has no args");

static_assert(select_call_traits<decltype(&y)>::arg_count == 1, "y argc count");
static_assert(std::is_same<select_call_traits<decltype(&y)>::arg_type<0>, int>::value, "y 1st arg");

static_assert(select_call_traits<decltype(&z)>::arg_count == 1, "z argc count");
static_assert(std::is_same<select_call_traits<decltype(&z)>::arg_type<0>, v8::Isolate*>::value, "z 1st arg");
static_assert(std::is_same<select_call_traits<decltype(&z)>::arg_type<1>, int>::value, "z 2nd arg");

static_assert(std::is_same<select_call_traits<decltype(&x)>, call_from_v8_traits<decltype(&x)>>::value, "");

static_assert(std::is_same<select_call_traits<decltype(&z)>, isolate_arg_call_traits<decltype(&z)>>::value, "");

static_assert(std::is_same<select_call_traits<decltype(&w)>, v8_args_call_traits<decltype(&w)>>::value, "");

static_assert(std::is_same<select_call_traits<decltype(&class_ptr)>::arg_convert<0, raw_ptr_traits>::from_type, X*>::value, "class ptr");
static_assert(std::is_same<select_call_traits<decltype(&class_ref)>::arg_convert<0, raw_ptr_traits>::from_type, X&>::value, "class ref");
static_assert(std::is_same<select_call_traits<decltype(&class_sptr)>::arg_convert<0, raw_ptr_traits>::from_type, std::shared_ptr<X>>::value, "class shared_ptr");

static_assert(std::is_same<select_call_traits<decltype(&class_ptr)>::arg_convert<0, shared_ptr_traits>::from_type, std::shared_ptr<X>>::value, "class ptr");
static_assert(std::is_same<select_call_traits<decltype(&class_ref)>::arg_convert<0, shared_ptr_traits>::from_type, X&>::value, "class ref");
static_assert(std::is_same<select_call_traits<decltype(&class_sptr)>::arg_convert<0, shared_ptr_traits>::from_type, std::shared_ptr<X>>::value, "class shared_ptr");

// fundamental type converters
static_assert(std::is_same<call_from_v8_traits<decltype(y)>::arg_convert<0, raw_ptr_traits>, v8pp::convert<int>>::value, "y(int)");
static_assert(std::is_same<call_from_v8_traits<decltype(y)>::arg_convert<0, shared_ptr_traits>, v8pp::convert<int>>::value, "y(int)");

static_assert(std::is_same<call_from_v8_traits<decltype(z)>::arg_convert<1, raw_ptr_traits>, v8pp::convert<int>>::value, "y(int)");
static_assert(std::is_same<call_from_v8_traits<decltype(z)>::arg_convert<1, shared_ptr_traits>, v8pp::convert<int>>::value, "y(int)");

// cv arg converters
static void s(std::string, std::vector<int>&, std::shared_ptr<int> const&, std::string*, std::string const*) {}

static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<0, raw_ptr_traits>, v8pp::convert<std::string>>::value, "s(string)");
static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<0, shared_ptr_traits>, v8pp::convert<std::string>>::value, "s(string)");

static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<1, raw_ptr_traits>, v8pp::convert<std::vector<int>>>::value, "s(vector<int>&)");
static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<1, shared_ptr_traits>, v8pp::convert<std::vector<int>>>::value, "s(vector<int>&)");

static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<2, raw_ptr_traits>, v8pp::convert<std::shared_ptr<int>>>::value, "s(std::shared_ptr<int> const&)");
static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<2, shared_ptr_traits>, v8pp::convert<std::shared_ptr<int>>>::value, "s(std::shared_ptr<int> const&)");

static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<3, raw_ptr_traits>, v8pp::convert<std::string*>>::value, "s(std::string*)");
static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<3, shared_ptr_traits>, v8pp::convert<std::string*>>::value, "s(std::string*)");

static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<4, raw_ptr_traits>, v8pp::convert<std::string const*>>::value, "s(std::string const*)");
static_assert(std::is_same<call_from_v8_traits<decltype(s)>::arg_convert<4, shared_ptr_traits>, v8pp::convert<std::string const*>>::value, "s(std::string const*)");

// fundamental types cv arg converters
static void t(int, char&, bool const&, float*, char const*) {}

static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<0, raw_ptr_traits>, v8pp::convert<int>>::value, "t(int)");
static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<0, shared_ptr_traits>, v8pp::convert<int>>::value, "t(int)");

static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<1, raw_ptr_traits>, v8pp::convert<char>>::value, "t(char&)");
static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<1, shared_ptr_traits>, v8pp::convert<char>>::value, "t(char&)");

static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<2, raw_ptr_traits>, v8pp::convert<bool>>::value, "t(bool const&)");
static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<2, shared_ptr_traits>, v8pp::convert<bool>>::value, "t(bool const&)");

static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<3, raw_ptr_traits>, v8pp::convert<float*>>::value, "t(float*)");
static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<3, shared_ptr_traits>, v8pp::convert<float*>>::value, "t(float*)");

static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<4, raw_ptr_traits>, v8pp::convert<char const*>>::value, "t(char const*)");
static_assert(std::is_same<call_from_v8_traits<decltype(t)>::arg_convert<4, shared_ptr_traits>, v8pp::convert<char const*>>::value, "t(char const*)");

void test_call_from_v8()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	context.set("x", v8pp::wrap_function(isolate, "x", &x));
	context.set("y", v8pp::wrap_function(isolate, "y", &y));
	context.set("z", v8pp::wrap_function(isolate, "z", &z));
	context.set("w", v8pp::wrap_function(isolate, "w", &w));

	check_eq("x", run_script<int>(context, "x()"), 0);
	check_eq("y", run_script<int>(context, "y(1)"), 1);
	check_eq("z", run_script<int>(context, "z(2)"), 2);
	check_eq("w", run_script<int>(context, "w(2, 'd', true, null)"), 4);
}
