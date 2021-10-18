#include "v8pp/property.hpp"
#include "v8pp/context.hpp"

#include "test.hpp"

int get1();
void set1(int);

int get2(v8::Isolate*);
void set2(v8::Isolate*, int);

void get3(v8::Local<v8::String> name,
	v8::PropertyCallbackInfo<v8::Value> const& info);
void set3(v8::Local<v8::String> name, v8::Local<v8::Value> value,
	v8::PropertyCallbackInfo<void> const& info);

struct X
{
	bool get1() const;
	void set1(int);

	bool get2(v8::Isolate*);
	void set2(v8::Isolate*, int);

	void get3(v8::Local<v8::String> name,
		v8::PropertyCallbackInfo<v8::Value> const& info);
	void set3(v8::Local<v8::String> name, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info);
};

using namespace v8pp::detail;

//property metafunctions
static_assert(is_getter<decltype(&get1)>::value, "getter function");
static_assert(is_getter<decltype(&X::get1)>::value, "getter member function");

static_assert(is_setter<decltype(&set1)>::value, "setter function");
static_assert(is_setter<decltype(&X::set1)>::value, "setter member function");

static_assert(is_isolate_getter<decltype(&get2)>::value, "isolate getter function");
static_assert(is_isolate_getter<decltype(&X::get2)>::value, "isolate getter member function");

static_assert(is_isolate_setter<decltype(&set2)>::value, "isolate setter function");
static_assert(is_isolate_setter<decltype(&X::set2)>::value, "isolate setter member function");

static_assert(is_direct_getter<decltype(&get3)>::value, "direct getter function");
static_assert(is_direct_getter<decltype(&X::get3)>::value, "direct getter member function");

static_assert(is_direct_setter<decltype(&set3)>::value, "direct setter function");
static_assert(is_direct_setter<decltype(&X::set3)>::value, "direct setter member function");

// tag selectors
static_assert(std::is_same<select_getter_tag<decltype(&get1)>, getter_tag>::value, "getter function tag");
static_assert(std::is_same<select_getter_tag<decltype(&X::get1)>, getter_tag>::value, "getter member function tag");

static_assert(std::is_same<select_setter_tag<decltype(&set1)>, setter_tag>::value, "setter function tag");
static_assert(std::is_same<select_setter_tag<decltype(&X::set1)>, setter_tag>::value, "setter member function tag");

static_assert(std::is_same<select_getter_tag<decltype(&get2)>, isolate_getter_tag>::value, "isolate getter function tag");
static_assert(std::is_same<select_getter_tag<decltype(&X::get2)>, isolate_getter_tag>::value, "isolate getter member function tag");

static_assert(std::is_same<select_setter_tag<decltype(&set2)>, isolate_setter_tag>::value, "isolate setter function tag");
static_assert(std::is_same<select_setter_tag<decltype(&X::set2)>, isolate_setter_tag>::value, "isolate setter member function tag");

static_assert(std::is_same<select_getter_tag<decltype(&get3)>, direct_getter_tag>::value, "direct getter function tag");
static_assert(std::is_same<select_getter_tag<decltype(&X::get3)>, direct_getter_tag>::value, "direct getter member function tag");

static_assert(std::is_same<select_setter_tag<decltype(&set3)>, direct_setter_tag>::value, "direct setter function tag");
static_assert(std::is_same<select_setter_tag<decltype(&X::set3)>, direct_setter_tag>::value, "direct setter member function tag");

void test_property()
{
}
