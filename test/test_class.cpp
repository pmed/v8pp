#include "v8pp/class.hpp"
#include "v8pp/property.hpp"

#include "test.hpp"

struct X
{
	int var = 1;

	int get() const { return var; }
	void set(int v) { var = v; }

	int fun(int x) { return var + x; }
	static int static_fun(int x) { return x; }
};

struct Y : X
{
	explicit Y(int x) { var = x; }
};

namespace v8pp {
template<>
struct factory<Y>
{
	static Y* create(v8::Isolate*, int x) { return new Y(x); }
	static void destroy(v8::Isolate*, Y* object) { delete object; }
};
} // v8pp

void test_class()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8pp::class_<X> X_class(isolate);
	X_class
		.ctor()
		.set_const("konst", 99)
		.set("var", &X::var)
		.set("rprop", v8pp::property(&X::get))
		.set("wprop", v8pp::property(&X::get, &X::set))
		.set("fun", &X::fun)
		.set("static_fun", &X::static_fun)
	;

	v8pp::class_<Y> Y_class(context.isolate());
	Y_class
		.inherit<X>()
		.ctor<int>()
		;

	context
		.set("X", X_class)
		.set("Y", Y_class);

	check_eq("X object", run_script<int>(context, "x = new X(); x.konst + x.var"), 100);
	check_eq("X::rprop", run_script<int>(context, "x = new X(); x.rprop"), 1);
	check_eq("X::wprop", run_script<int>(context, "x = new X(); ++x.wprop"), 2);
	check_eq("X::fun(1)", run_script<int>(context, "x = new X(); x.fun(1)"), 2);
	check_eq("X::static_fun(1)", run_script<int>(context, "X.static_fun(3)"), 3);

	check_eq("Y object", run_script<int>(context, "y = new Y(-100); y.konst + y.var"), -1);
}
