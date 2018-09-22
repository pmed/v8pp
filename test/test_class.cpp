//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/class.hpp"
#include "v8pp/module.hpp"
#include "v8pp/object.hpp"
#include "v8pp/property.hpp"

#include "test.hpp"

struct Xbase
{
	int var = 1;

	int get() const { return var; }
	void set(int v) { var = v; }

	int prop() const { return var; }
	void prop(int v) { var = v; }

	int fun1(int x) { return var + x; }
	int fun2(int x) const { return var + x; }
	int fun3(int x) volatile { return var + x; }
	int fun4(int x) const volatile { return var + x; }
	static int static_fun(int x) { return x; }

	v8::Local<v8::Value> to_json(v8::Isolate* isolate, v8::Local<v8::Value> key) const
	{
		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Object> result = v8::Object::New(isolate);
		v8pp::set_const(isolate, result, "key", key);
		v8pp::set_const(isolate, result, "var", var);
		return scope.Escape(result);
	}
};

struct X : Xbase
{
};

template<typename Traits, typename X_ptr = typename v8pp::class_<X, Traits>::object_pointer_type>
static X_ptr create_X(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	X_ptr x(new X);
	if (args.Length() > 0)
	{
		x->var = v8pp::from_v8<int>(args.GetIsolate(), args[0]);
	}
	return x;
}

struct Y : X
{
	static int instance_count;

	explicit Y(int x) { var = x; ++instance_count; }
	~Y() { --instance_count; }

	int useX(X& x) { return var + x.var; }

	template<typename Traits, typename X_ptr = typename v8pp::class_<X, Traits>::object_pointer_type>
	int useX_ptr(X_ptr x) { return var + x->var; }
};

int Y::instance_count = 0;

struct Z {};

namespace v8pp {
template<>
struct factory<Y, v8pp::raw_ptr_traits>
{
	static Y* create(v8::Isolate*, int x) { return new Y(x); }
	static void destroy(v8::Isolate*, Y* object) { delete object; }
};
template<>
struct factory<Y, v8pp::shared_ptr_traits>
{
	static std::shared_ptr<Y> create(v8::Isolate*, int x)
	{
		return std::make_shared<Y>(x);
	}
	static void destroy(v8::Isolate*, std::shared_ptr<Y> const&) {}
};
} // v8pp

template<typename Traits>
static int extern_fun(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	int x = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromJust();
	auto self = v8pp::class_<X, Traits>::unwrap_object(args.GetIsolate(), args.This());
	if (self) x += self->var;
	return x;
}

template<typename Traits>
void test_class_()
{
	Y::instance_count = 0;

	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	using x_prop_get = int (X::*)() const;
	using x_prop_set = void (X::*)(int);

	int extra_ctor_context = 1;
	auto const X_ctor = [extra_ctor_context](v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		return create_X<Traits>(args);
	};
	Z extra_dtor_context;
	auto const X_dtor = [extra_dtor_context](v8::Isolate* isolate, typename Traits::template object_pointer_type<X> const& obj)
	{
		Traits::destroy(obj);
	};

	v8pp::class_<X, Traits> X_class(isolate, X_dtor);
	X_class
		.ctor(&create_X<Traits>)
		.template ctor<v8::FunctionCallbackInfo<v8::Value> const&>(X_ctor)
		.set_const("konst", 99)
		.set("var", &X::var)
		.set("rprop", v8pp::property(&X::get))
		.set("wprop", v8pp::property(&X::get, &X::set))
		.set("wprop2", v8pp::property(
			static_cast<x_prop_get>(&X::prop),
			static_cast<x_prop_set>(&X::prop)))
		.set("fun1", &X::fun1)
		.set("fun2", &X::fun2)
		.set("fun3", &X::fun3)
		.set("fun4", &X::fun4)
		.set("static_fun", &X::static_fun)
		.set("static_lambda", [](int x) { return x + 3; })
		.set("extern_fun", extern_fun<Traits>)
		.set("toJSON", &X::to_json)
		;

	v8pp::class_<Y, Traits> Y_class(isolate);
	Y_class
		.template inherit<X>()
		.template ctor<int>()
		.set("useX", &Y::useX)
		.set("useX_ptr", &Y::useX_ptr<Traits>)
		;

	check_ex<std::runtime_error>("already wrapped class X", [isolate]()
	{
		v8pp::class_<X, Traits> X_class(isolate);
	});
	check_ex<std::runtime_error>("already inherited class X", [&Y_class]()
	{
		Y_class.template inherit<X>();
	});
	check_ex<std::runtime_error>("unwrapped class Z", [isolate]()
	{
		v8pp::class_<Z, Traits>::find_object(isolate, nullptr);
	});

	context
		.set("X", X_class)
		.set("Y", Y_class)
		;

	check_eq("X object", run_script<int>(context, "x = new X(); x.var += x.konst"), 100);
	check_eq("X::rprop", run_script<int>(context, "x = new X(); x.rprop"), 1);
	check_eq("X::wprop", run_script<int>(context, "x = new X(); ++x.wprop"), 2);
	check_eq("X::wprop2", run_script<int>(context, "x = new X(); ++x.wprop2"), 2);
	check_eq("X::fun1(1)", run_script<int>(context, "x = new X(); x.fun1(1)"), 2);
	check_eq("X::fun2(2)", run_script<int>(context, "x = new X(); x.fun2(2)"), 3);
	check_eq("X::fun3(3)", run_script<int>(context, "x = new X(); x.fun3(3)"), 4);
	check_eq("X::fun4(4)", run_script<int>(context, "x = new X(); x.fun4(4)"), 5);
	check_eq("X::static_fun(1)", run_script<int>(context, "X.static_fun(1)"), 1);
	check_eq("X::static_lambda(1)", run_script<int>(context, "X.static_lambda(1)"), 4);
	check_eq("X::extern_fun(5)", run_script<int>(context, "x = new X(); x.extern_fun(5)"), 6);
	check_eq("X::extern_fun(6)", run_script<int>(context, "X.extern_fun(6)"), 6);

	check_eq("JSON.stringify(X)",
		run_script<std::string>(context, "JSON.stringify({'obj': new X(10), 'arr': [new X(11), new X(12)] })"),
		R"({"obj":{"key":"obj","var":10},"arr":[{"key":"0","var":11},{"key":"1","var":12}]})"
	);

	check_eq("Y object", run_script<int>(context, "y = new Y(-100); y.konst + y.var"), -1);

	auto y1 = v8pp::factory<Y, Traits>::create(isolate, -1);

	v8::Local<v8::Object> y1_obj =
		v8pp::class_<Y, Traits>::reference_external(context.isolate(), y1);
	check("y1", v8pp::from_v8<decltype(y1)>(isolate, y1_obj) == y1);
	check("y1_obj", v8pp::to_v8(isolate, y1) == y1_obj);

	auto y2 = v8pp::factory<Y, Traits>::create(isolate, -2);
	v8::Local<v8::Object> y2_obj =
		v8pp::class_<Y, Traits>::import_external(context.isolate(), y2);
	check("y2", v8pp::from_v8<decltype(y2)>(isolate, y2_obj) == y2);
	check("y2_obj", v8pp::to_v8(isolate, y2) == y2_obj);

	v8::Local<v8::Object> y3_obj =
		v8pp::class_<Y, Traits>::create_object(context.isolate(), -3);
	auto y3 = v8pp::class_<Y, Traits>::unwrap_object(isolate, y3_obj);
	check("y3", v8pp::from_v8<decltype(y3)>(isolate, y3_obj) == y3);
	check("y3_obj", v8pp::to_v8(isolate, y3) == y3_obj);
	check_eq("y3.var", y3->var, -3);

	run_script<int>(context, "x = new X; for (i = 0; i < 10; ++i) { y = new Y(i); y.useX(x); y.useX_ptr(x); }");
	check_eq("Y count", Y::instance_count, 10 + 4); // 10 + y + y1 + y2 + y3
	run_script<int>(context, "y = null; 0");

	v8pp::class_<Y, Traits>::unreference_external(isolate, y1);
	check("unref y1", !v8pp::from_v8<decltype(y1)>(isolate, y1_obj));
	check("unref y1_obj", v8pp::to_v8(isolate, y1).IsEmpty());
	y1_obj.Clear();
	check_ex<std::runtime_error>("y1 unreferenced", [isolate, &y1]()
	{
		v8pp::to_v8(isolate, y1);
	});

	v8pp::class_<Y, Traits>::destroy_object(isolate, y2);
	check("unref y2", !v8pp::from_v8<decltype(y2)>(isolate, y2_obj));
	check("unref y2_obj", v8pp::to_v8(isolate, y2).IsEmpty());
	y2_obj.Clear();

	v8pp::class_<Y, Traits>::destroy_object(isolate, y3);
	check("unref y3", !v8pp::from_v8<decltype(y3)>(isolate, y3_obj));
	check("unref y3_obj", v8pp::to_v8(isolate, y3).IsEmpty());
	y3_obj.Clear();

	std::string const v8_flags = "--expose_gc";
	v8::V8::SetFlagsFromString(v8_flags.data(), (int)v8_flags.length());
	context.isolate()->RequestGarbageCollectionForTesting(
		v8::Isolate::GarbageCollectionType::kFullGarbageCollection);

	bool const use_shared_ptr = std::is_same<Traits, v8pp::shared_ptr_traits>::value;

	check_eq("Y count after GC", Y::instance_count,
		1 + 2 * use_shared_ptr); // y1 + (y2 + y3 when use_shared_ptr)

	y1_obj = v8pp::class_<Y, Traits>::reference_external(context.isolate(), y1);

	check_eq("Y count before class_<Y>::destroy", Y::instance_count,
		1 + 2 * use_shared_ptr); // y1 + (y2 + y3 when use_shared_ptr)
	v8pp::class_<Y, Traits>::destroy(isolate);
	check_eq("Y count after class_<Y>::destroy", Y::instance_count,
		1 + 2 * use_shared_ptr); // y1 + (y2 + y3 when use_shared_ptr)
}

template<typename Traits>
void test_multiple_inheritance()
{
	struct A
	{
		int x;
		A() : x(1) {}
		int f() { return x; }
		void set_f(int v) { x = v; }

		int z() const { return x; }
	};

	struct B
	{
		int x;
		B() : x(2) {}
		int g() { return x; }
		void set_g(int v) { x = v; }

		int z() const { return x; }
	};

	struct C : A, B
	{
		int x;
		C() : x(3) {}
		int h() { return x; }
		void set_h(int v) { x = v; }

		int z() const { return x; }
	};

	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8pp::class_<B, Traits> B_class(isolate);
	B_class
		.set("xB", &B::x)
		.set("zB", &B::z)
		.set("g", &B::g);

	v8pp::class_<C, Traits> C_class(isolate);
	C_class
		.template inherit<B>()
		.template ctor<>()
		.set("xA", &A::x)
		.set("xC", &C::x)

		.set("zA", &A::z)
		.set("zC", &C::z)

		.set("f", &A::f)
		.set("h", &C::h)

		.set("rF", v8pp::property(&C::f))
		.set("rG", v8pp::property(&C::g))
		.set("rH", v8pp::property(&C::h))

		.set("F", v8pp::property(&C::f, &C::set_f))
		.set("G", v8pp::property(&C::g, &C::set_g))
		.set("H", v8pp::property(&C::h, &C::set_h))
		;


	context.set("C", C_class);
	check_eq("get attributes", run_script<int>(context, "c = new C(); c.xA + c.xB + c.xC"), 1 + 2 + 3);
	check_eq("set attributes", run_script<int>(context,
		"c = new C(); c.xA = 10; c.xB = 20; c.xC = 30; c.xA + c.xB + c.xC"), 10 + 20 + 30);

	check_eq("functions", run_script<int>(context, "c = new C(); c.f() + c.g() + c.h()"), 1 + 2 + 3);
	check_eq("z functions", run_script<int>(context, "c = new C(); c.zA() + c.zB() + c.zC()"), 1 + 2 + 3);

	check_eq("rproperties", run_script<int>(context,
		"c = new C(); c.rF + c.rG + c.rH"), 1 + 2 + 3);
	check_eq("rwproperties", run_script<int>(context,
		"c = new C(); c.F = 100; c.G = 200; c.H = 300; c.F + c.G + c.H"), 100 + 200 + 300);
}

template<typename Traits>
void test_const_instance_in_module()
{
	struct X
	{
		int f(int x) { return x; }
		static typename Traits::template object_pointer_type<X> xconst()
		{
			static auto const xconst_ = v8pp::factory<X, Traits>::create(v8::Isolate::GetCurrent());
			return xconst_;
		}
	};

	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8pp::class_<X, Traits> X_class(isolate);
	X_class.set("f", &X::f);

	v8pp::module api(isolate);
	api.set("X", X_class);
	api.set("xconst", v8pp::property(X::xconst));

	X_class.reference_external(isolate, X::xconst());

	context.set("api", api);
	check_eq("xconst.f()", run_script<int>(context, "api.xconst.f(1)"), 1);
}

void test_class()
{
	test_class_<v8pp::raw_ptr_traits>();
	test_class_<v8pp::shared_ptr_traits>();

	test_multiple_inheritance<v8pp::raw_ptr_traits>();
	test_multiple_inheritance<v8pp::shared_ptr_traits>();

	test_const_instance_in_module<v8pp::raw_ptr_traits>();
	test_const_instance_in_module<v8pp::shared_ptr_traits>();
}
