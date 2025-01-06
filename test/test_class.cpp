#include "v8pp/class.hpp"
#include "v8pp/json.hpp"
#include "v8pp/module.hpp"
#include "v8pp/object.hpp"
#include "v8pp/property.hpp"

#include "test.hpp"

#include <type_traits>

struct Xbase
{
	char c = 'X';
	int var = 1;

	int get() const { return var; }
	void set(int v) { var = v; }

	int prop() const { return var; }
	void prop(int v) { var = v; }

	int fun1(int x) { return var + x; }
	float fun2(float x) const { return var + x; }

	std::string fun3(std::string const& x) volatile
	{
		return x + std::to_string(var);
	}

	std::vector<std::string> fun4(std::vector<std::string> x) const volatile
	{
		x.push_back(std::to_string(var));
		return x;
	}

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
	X_ptr x = Traits::template create<X>();
	switch (args.Length())
	{
	case 1:
		x->var = v8pp::from_v8<int>(args.GetIsolate(), args[0]);
		break;
	case 2:
		throw std::runtime_error("C++ exception");
	case 3:
		v8pp::throw_ex(args.GetIsolate(), "JS exception");
	}
	return x;
}

int external_get1(X const volatile& x)
{
	return x.var;
}

int external_get2(X const& x, v8::Isolate*)
{
	return x.var;
}

void external_get3(X const& x, v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	info.GetReturnValue().Set(v8pp::to_v8(info.GetIsolate(), x.var));
}

void external_set1(X volatile& x, int value)
{
	x.var = value;
}

void external_set2(X& x, v8::Isolate*, int value)
{
	x.var = value;
}

void external_set3(X& x, v8::Local<v8::Name>, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
{
	x.var = v8pp::from_v8<int>(info.GetIsolate(), value);
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

struct Z
{
};

template<typename Traits>
static int extern_fun(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	int x = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromJust();
	auto self = v8pp::class_<X, Traits>::unwrap_object(args.GetIsolate(), args.This());
	if (self) x += self->var;
	return x;
}

template<typename Traits>
void get_rprop_direct(v8::Local<v8::Name>, v8::PropertyCallbackInfo<v8::Value> const& info)
{
	auto self = v8pp::class_<X, Traits>::unwrap_object(info.GetIsolate(), info.This());
	info.GetReturnValue().Set(v8pp::to_v8(info.GetIsolate(), self->var));
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
		(void)extra_ctor_context;
		return create_X<Traits>(args);
	};

	Z extra_dtor_context;
	auto const X_dtor = [extra_dtor_context](v8::Isolate*, typename Traits::template object_pointer_type<X> const& obj)
	{
		(void)extra_dtor_context;
		Traits::destroy(obj);
	};

	v8pp::class_<X, Traits> X_class(isolate, X_dtor);
	X_class
		.ctor(&create_X<Traits>)
		.template ctor<v8::FunctionCallbackInfo<v8::Value> const&>(X_ctor)
		.const_("konst", 99)
		.var("var", &X::var)
		//TODO: static property definition works only at the end of class_ declaration!
		//.static_("my_static_var", 1)
		//.static_("my_static_const_var", 42, true)
		.property("rprop", &X::get)
		.property("wprop", &X::get, &X::set)
		.property("wprop2", static_cast<x_prop_get>(&X::prop), static_cast<x_prop_set>(&X::prop))
		.property("prop", [](X const& x) mutable { return x.var; }, [](X& x, int n) { x.var = n; })
		.property("prop2", [](X const& x) { return x.var; }, [](X& x, int n) mutable { x.var = n; })
		.property("rprop_direct", &get_rprop_direct<Traits>)
		.property("rprop_external1", &external_get1)
		.property("rprop_external2", &external_get2)
		.property("rprop_external3", &external_get3)
		.property("wprop_external1", &external_get1, &external_set1)
		.property("wprop_external2", &external_get2, &external_set2)
		.property("wprop_external3", &external_get3, &external_set3)
		.function("fun1", &X::fun1)
		.function("fun2", &X::fun2)
		.function("fun3", &X::fun3)
		.function("fun4", &X::fun4)
		.function("static_fun", &X::static_fun)
		.function("static_lambda", [](int x) { return x + 3; })
		.function("extern_fun", &extern_fun<Traits>)
		.function("toJSON", &X::to_json)
		.static_("my_static_var", 1)
		.static_("my_static_const_var", 42, true)
		;

	static_assert(std::is_move_constructible_v<decltype(X_class)>);
	static_assert(!std::is_move_assignable_v<decltype(X_class)>);
	static_assert(!std::is_copy_assignable_v<decltype(X_class)>);
	static_assert(!std::is_copy_constructible_v<decltype(X_class)>);

	v8pp::class_<Y, Traits> Y_class(isolate);
	Y_class
		.template inherit<X>()
		.template ctor<int>()
		.function("useX", &Y::useX)
		.function("useX_ptr", &Y::useX_ptr<Traits>);

	auto Y_class_find = v8pp::class_<Y, Traits>::extend(isolate);
	Y_class_find.function("toJSON", [](const v8::FunctionCallbackInfo<v8::Value>& args)
	{
		bool const with_functions = true;
		args.GetReturnValue().Set(v8pp::json_object(args.GetIsolate(), args.This(), with_functions));
	});

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
		.class_("X", X_class)
		.class_("Y", Y_class)
		;

	check_eq("C++ exception from X ctor",
		run_script<std::string>(context, "ret = ''; try { new X(1, 2); } catch(err) { ret = err.message; } ret"),
		"C++ exception");
	check("Unhandled C++ exception from X ctor", context.run_script("x = new X(1, 2); x").IsEmpty());
	check_eq("V8 exception from X ctor",
		run_script<std::string>(context, "ret = ''; try { new X(1, 2, 3); } catch(err) { ret = err.message; } ret"),
		"JS exception");
	check("Unhandled V8 exception from X ctor", context.run_script("x = new X(1, 2, 3); x").IsEmpty());

	check_eq("X object", run_script<int>(context, "x = new X(); x.var += x.konst"), 100);
	check_eq("X::rprop", run_script<int>(context, "x = new X(); x.rprop"), 1);
	check_eq("X::wprop", run_script<int>(context, "x = new X(); ++x.wprop"), 2);
	check_eq("X::wprop2", run_script<int>(context, "x = new X(); ++x.wprop2"), 2);
	check_eq("X::prop", run_script<int>(context, "x = new X(); x.prop += 2"), 3);
	check_eq("X::prop2", run_script<int>(context, "x = new X(); x.prop2 += 3"), 4);
	check_eq("X::rprop_direct", run_script<int>(context, "x = new X(); x.rprop_direct"), 1);
	check_eq("X::rprop_external1", run_script<int>(context, "x = new X(); x.rprop_external1"), 1);
	check_eq("X::rprop_external2", run_script<int>(context, "x = new X(); x.rprop_external2"), 1);
	check_eq("X::rprop_external3", run_script<int>(context, "x = new X(); x.rprop_external3"), 1);
	check_eq("X::wprop_external1", run_script<int>(context, "x = new X(); ++x.wprop_external1; x.wprop_external1"), 2);
	check_eq("X::wprop_external2", run_script<int>(context, "x = new X(); ++x.wprop_external2; x.wprop_external2"), 2);
	check_eq("X::wprop_external3", run_script<int>(context, "x = new X(); ++x.wprop_external3; x.wprop_external3"), 2);
	check_eq("X::fun1(1)", run_script<int>(context, "x = new X(); x.fun1(1)"), 2);
	check_eq("X::fun2(2)", run_script<float>(context, "x = new X(); x.fun2(2)"), 3);
	check_eq("X::fun3(str)", run_script<std::string>(context, "x = new X(); x.fun3('str')"), "str1");
	check_eq("X::fun4([foo, bar])",
		run_script<std::vector<std::string>>(context, "x = new X(); x.fun4(['foo', 'bar'])"),
		std::vector<std::string>{{ "foo", "bar", "1" }});
	check_eq("X::static_fun(1)", run_script<int>(context, "X.static_fun(1)"), 1);
	check_eq("X::static_lambda(1)", run_script<int>(context, "X.static_lambda(1)"), 4);
	check_eq("X::extern_fun(5)", run_script<int>(context, "x = new X(); x.extern_fun(5)"), 6);
	check_eq("X::extern_fun(6)", run_script<int>(context, "X.extern_fun(6)"), 6);
	check_eq("X::my_static_const_var", run_script<int>(context, "X.my_static_const_var"), 42);
	check_eq("X::my_static_const_var after assign", run_script<int>(context, "X.my_static_const_var = 123; X.my_static_const_var"), 42);
	check_eq("X::my_static_var", run_script<int>(context, "X.my_static_var"), 1);
	check_eq("X::my_static_var after assign", run_script<int>(context, "X.my_static_var = 123; X.my_static_var"), 123);

	check_ex<std::runtime_error>("call method with invalid instance", [&context]()
	{
		run_script<int>(context, "x = new X(); f = x.fun1; f(1)");
	});

	check_eq("JSON.stringify(X)",
		run_script<std::string>(context, "JSON.stringify({'obj': new X(10), 'arr': [new X(11), new X(12)] })"),
		R"({"obj":{"key":"obj","var":10},"arr":[{"key":"0","var":11},{"key":"1","var":12}]})"
	);

	check_eq("JSON.stringify(Y)",
		run_script<std::string>(context, "JSON.stringify({'obj': new Y(10), 'arr': [new Y(11), new Y(12)] })"),
		R"({"obj":{"useX":"function useX() { [native code] }","useX_ptr":"function useX_ptr() { [native code] }","toJSON":"function toJSON() { [native code] }","wprop_external3":10,"wprop_external2":10,"wprop_external1":10,"rprop_external3":10,"rprop_external2":10,"rprop_external1":10,"rprop_direct":10,"prop2":10,"prop":10,"wprop2":10,"wprop":10,"rprop":10,"var":10,"konst":99,"fun1":"function fun1() { [native code] }","fun2":"function fun2() { [native code] }","fun3":"function fun3() { [native code] }","fun4":"function fun4() { [native code] }","static_fun":"function static_fun() { [native code] }","static_lambda":"function static_lambda() { [native code] }","extern_fun":"function extern_fun() { [native code] }"},"arr":[{"useX":"function useX() { [native code] }","useX_ptr":"function useX_ptr() { [native code] }","toJSON":"function toJSON() { [native code] }","wprop_external3":11,"wprop_external2":11,"wprop_external1":11,"rprop_external3":11,"rprop_external2":11,"rprop_external1":11,"rprop_direct":11,"prop2":11,"prop":11,"wprop2":11,"wprop":11,"rprop":11,"var":11,"konst":99,"fun1":"function fun1() { [native code] }","fun2":"function fun2() { [native code] }","fun3":"function fun3() { [native code] }","fun4":"function fun4() { [native code] }","static_fun":"function static_fun() { [native code] }","static_lambda":"function static_lambda() { [native code] }","extern_fun":"function extern_fun() { [native code] }"},{"useX":"function useX() { [native code] }","useX_ptr":"function useX_ptr() { [native code] }","toJSON":"function toJSON() { [native code] }","wprop_external3":12,"wprop_external2":12,"wprop_external1":12,"rprop_external3":12,"rprop_external2":12,"rprop_external1":12,"rprop_direct":12,"prop2":12,"prop":12,"wprop2":12,"wprop":12,"rprop":12,"var":12,"konst":99,"fun1":"function fun1() { [native code] }","fun2":"function fun2() { [native code] }","fun3":"function fun3() { [native code] }","fun4":"function fun4() { [native code] }","static_fun":"function static_fun() { [native code] }","static_lambda":"function static_lambda() { [native code] }","extern_fun":"function extern_fun() { [native code] }"}]})"
	);

	check_eq("Y object", run_script<int>(context, "y = new Y(-100); y.konst + y.var"), -1);

	auto y1 = Traits::template create<Y>(-1);

	v8::Local<v8::Object> y1_obj = v8pp::class_<Y, Traits>::reference_external(context.isolate(), y1);
	check("y1", v8pp::from_v8<decltype(y1)>(isolate, y1_obj) == y1);
	check("y1_obj", v8pp::to_v8(isolate, y1) == y1_obj);

	auto y2 = Traits::template create<Y>(-2);
	v8::Local<v8::Object> y2_obj = v8pp::class_<Y, Traits>::import_external(context.isolate(), y2);
	check("y2", v8pp::from_v8<decltype(y2)>(isolate, y2_obj) == y2);
	check("y2_obj", v8pp::to_v8(isolate, y2) == y2_obj);

	v8::Local<v8::Object> y3_obj = v8pp::class_<Y, Traits>::create_object(context.isolate(), -3);
	auto y3 = v8pp::class_<Y, Traits>::unwrap_object(isolate, y3_obj);
	check("y3", v8pp::from_v8<decltype(y3)>(isolate, y3_obj) == y3);
	check("y3_obj", v8pp::to_v8(isolate, y3) == y3_obj);
	check_eq("y3.var", y3->var, -3);

	run_script<int>(context, "x = new X; for (i = 0; i < 10; ++i) { y = new Y(i); y.useX(x); y.useX_ptr(x); }");
	check_eq("Y count", Y::instance_count, 13 + 4); // 13 + y + y1 + y2 + y3
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

	context.isolate()->RequestGarbageCollectionForTesting(
		v8::Isolate::GarbageCollectionType::kFullGarbageCollection);

	bool constexpr use_shared_ptr = std::same_as<Traits, v8pp::shared_ptr_traits>;

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
		char a = 'A';
		int x;
		A() : x(1) {}
		int f() { return x; }
		void set_f(int v) { x = v; }

		int z() const { return x; }
	};

	struct B
	{
		char b = 'B';
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
		.var("xB", &B::x)
		.function("zB", &B::z)
		.function("g", &B::g);

	v8pp::class_<C, Traits> C_class(isolate);
	C_class
		.template inherit<B>()
		.template ctor<>()
		.var("xA", &A::x)
		.var("xC", &C::x)

		.function("zA", &A::z)
		.function("zC", &C::z)

		.function("f", &A::f)
		.function("h", &C::h)

		.property("rF", &C::f)
		.property("rG", &C::g)
		.property("rH", &C::h)

		.property("F", &C::f, &C::set_f)
		.property("G", &C::g, &C::set_g)
		.property("H", &C::h, &C::set_h)
		;

	context.class_("C", C_class);
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
			static auto const xconst_ = Traits::template create<X>();
			return xconst_;
		}
	};

	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8pp::class_<X, Traits> X_class(isolate);
	X_class.function("f", &X::f);

	v8pp::module api(isolate);
	api.class_("X", X_class);
	api.property("xconst", &X::xconst);

	X_class.reference_external(isolate, X::xconst());

	context.module("api", api);
	check_eq("xconst.f()", run_script<int>(context, "api.xconst.f(1)"), 1);
}

template<typename Traits>
void test_auto_wrap_objects()
{
	struct X
	{
		int x;
		explicit X(int x) : x(x) {}
		int get_x() const { return x; }
	};

	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8pp::class_<X, Traits> X_class(isolate);
	X_class
		.template ctor<int>()
		.auto_wrap_objects(true)
		.property("x", &X::get_x)
		;

	auto f = [](int x) { return X(x); };

	context.class_("X", X_class);
	context.function<decltype(f), Traits>("f", std::move(f));
	check_eq("return X object", run_script<int>(context, "obj = f(123); obj.x"), 123);
}

void test_class()
{
	test_class_<v8pp::raw_ptr_traits>();
	test_class_<v8pp::shared_ptr_traits>();

	test_multiple_inheritance<v8pp::raw_ptr_traits>();
	test_multiple_inheritance<v8pp::shared_ptr_traits>();

	test_const_instance_in_module<v8pp::raw_ptr_traits>();
	test_const_instance_in_module<v8pp::shared_ptr_traits>();

	test_auto_wrap_objects<v8pp::raw_ptr_traits>();
	test_auto_wrap_objects<v8pp::shared_ptr_traits>();
}
