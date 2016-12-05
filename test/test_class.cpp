//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/class.hpp"
#include "v8pp/property.hpp"

#include "test.hpp"

struct Xbase
{
	int var = 1;

	int get() const { return var; }
	void set(int v) { var = v; }

	int fun1(int x) { return var + x; }
	int fun2(int x) const { return var + x; }
	int fun3(int x) volatile { return var + x; }
	int fun4(int x) const volatile { return var + x; }
	static int static_fun(int x) { return x; }
};

struct X : Xbase
{
};

struct Y : X
{
	static int instance_count;

	explicit Y(int x) { var = x; ++instance_count; }
	~Y() { --instance_count; }
};

int Y::instance_count = 0;

struct Z {};

namespace v8pp {
template<>
struct factory<Y>
{
	static Y* create(v8::Isolate*, int x) { return new Y(x); }
	static void destroy(v8::Isolate*, Y* object) { delete object; }
};
template<>
struct factory<Y, true>
{
	static std::shared_ptr<Y> create(v8::Isolate*, int x)
	{
		return std::make_shared<Y>(x);
	}
	static void destroy(v8::Isolate*, std::shared_ptr<Y> const&) {}
};
} // v8pp

template<bool use_shared_ptr>
void test_class_()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8pp::class_<X, use_shared_ptr> X_class(isolate);
	X_class
		.ctor()
		.set_const("konst", 99)
		.set("var", &X::var)
		.set("rprop", v8pp::property(&X::get))
		.set("wprop", v8pp::property(&X::get, &X::set))
		.set("fun1", &X::fun1)
		.set("fun2", &X::fun2)
		.set("fun3", &X::fun3)
		.set("fun4", &X::fun4)
		.set("static_fun", &X::static_fun)
		.set("static_lambda", [](int x) { return x + 3; })
	;

	v8pp::class_<Y, use_shared_ptr> Y_class(context.isolate());
	Y_class
		.template inherit<X>()
		.template ctor<int>()
		;

	check_ex<std::runtime_error>("already wrapped class X", [isolate]()
	{
		v8pp::class_<X, use_shared_ptr> X_class(isolate);
	});
	check_ex<std::runtime_error>("already inherited class X", [isolate, &Y_class]()
	{
		Y_class.template inherit<X>();
	});
	check_ex<std::runtime_error>("unwrapped class Z", [isolate]()
	{
		v8pp::class_<Z, use_shared_ptr>::find_object(isolate, nullptr);
	});

	context
		.set("X", X_class)
		.set("Y", Y_class)
		;

	check_eq("X object", run_script<int>(context, "x = new X(); x.var += x.konst"), 100);
	check_eq("X::rprop", run_script<int>(context, "x = new X(); x.rprop"), 1);
	check_eq("X::wprop", run_script<int>(context, "x = new X(); ++x.wprop"), 2);
	check_eq("X::fun1(1)", run_script<int>(context, "x = new X(); x.fun1(1)"), 2);
	check_eq("X::fun2(2)", run_script<int>(context, "x = new X(); x.fun2(2)"), 3);
	check_eq("X::fun3(3)", run_script<int>(context, "x = new X(); x.fun3(3)"), 4);
	check_eq("X::fun4(4)", run_script<int>(context, "x = new X(); x.fun4(4)"), 5);
	check_eq("X::static_fun(1)", run_script<int>(context, "X.static_fun(3)"), 3);
	check_eq("X::static_lambda(1)", run_script<int>(context, "X.static_lambda(1)"), 4);

	check_eq("Y object", run_script<int>(context, "y = new Y(-100); y.konst + y.var"), -1);

	auto y1 = v8pp::factory<Y, use_shared_ptr>::create(isolate, -1);

	v8::Handle<v8::Object> y1_obj =
		v8pp::class_<Y, use_shared_ptr>::reference_external(context.isolate(), y1);
	check("y1", v8pp::from_v8<decltype(y1)>(isolate, y1_obj) == y1);
	check("y1_obj", v8pp::to_v8(isolate, y1) == y1_obj);

	auto y2 = v8pp::factory<Y, use_shared_ptr>::create(isolate, -2);
	v8::Handle<v8::Object> y2_obj =
		v8pp::class_<Y, use_shared_ptr>::import_external(context.isolate(), y2);
	check("y2", v8pp::from_v8<decltype(y2)>(isolate, y2_obj) == y2);
	check("y2_obj", v8pp::to_v8(isolate, y2) == y2_obj);

	v8::Handle<v8::Object> y3_obj =
		v8pp::class_<Y, use_shared_ptr>::create_object(context.isolate(), -3);
	auto y3 = v8pp::class_<Y, use_shared_ptr>::unwrap_object(isolate, y3_obj);
	check("y3", v8pp::from_v8<decltype(y3)>(isolate, y3_obj) == y3);
	check("y3_obj", v8pp::to_v8(isolate, y3) == y3_obj);
	check_eq("y3.var", y3->var, -3);

	run_script<int>(context, "for (i = 0; i < 10; ++i) new Y(i); i");
	check_eq("Y count", Y::instance_count, 10 + 4); // 10 + y + y1 + y2 + y3
	run_script<int>(context, "y = null; 0");

	v8pp::class_<Y, use_shared_ptr>::unreference_external(isolate, y1);
	check("unref y1", !v8pp::from_v8<decltype(y1)>(isolate, y1_obj));
	check("unref y1_obj", v8pp::to_v8(isolate, y1).IsEmpty());
	y1_obj.Clear();

	v8pp::class_<Y, use_shared_ptr>::destroy_object(isolate, y2);
	check("unref y2", !v8pp::from_v8<decltype(y2)>(isolate, y2_obj));
	check("unref y2_obj", v8pp::to_v8(isolate, y2).IsEmpty());
	y2_obj.Clear();

	v8pp::class_<Y, use_shared_ptr>::destroy_object(isolate, y3);
	check("unref y3", !v8pp::from_v8<decltype(y3)>(isolate, y3_obj));
	check("unref y3_obj", v8pp::to_v8(isolate, y3).IsEmpty());
	y3_obj.Clear();

	std::string const v8_flags = "--expose_gc";
	v8::V8::SetFlagsFromString(v8_flags.data(), (int)v8_flags.length());
	context.isolate()->RequestGarbageCollectionForTesting(
		v8::Isolate::GarbageCollectionType::kFullGarbageCollection);

	check_eq("Y count after GC", Y::instance_count,
		1 + 2 * use_shared_ptr); // y1 + (y2 + y3 when use_shared_ptr)

	v8pp::class_<Y, use_shared_ptr>::destroy(isolate);
	check_eq("Y count after destroy", Y::instance_count,
		1 + 2 * use_shared_ptr); // y1 + (y2 + y3 when use_shared_ptr)

	Y::instance_count = 0;
}

void test_class()
{
	test_class_<false>();
	test_class_<true>();
}
