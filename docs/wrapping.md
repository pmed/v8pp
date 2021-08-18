# Wrapping C++ code

V8 library uses `v8::ObjectTemplate` to bind C++ objects into JavaScript
and `v8::FunctionTemplate` to call C++ functions from JavaScript.

## Wrapping C++ functions

To create a new `v8::FunctionTemplate` for an arbitrary C++ function `func`
use a function template `v8::Local<v8::FunctionTemplate> v8pp::wrap_function_template(v8::Isolate* isolate, F func)`

When a function generated from such a function template is being invoked in
JavaScript, all function arguments will be converted from `v8::Value`s to
corresponding C++ values. Then the C++ function `func` will be called and its
return type would be converted back to `v8::Value`. For the C++ function
that returns `void` a `v8::Undefined` will be returned.

If the wrapped C++ function throws an exception, a `v8::Exception::Error` will
be returned into calling JavaScript code.

A function `v8::Local<v8::Function> v8pp::wrap_function(v8::Isolate* isolate, char const* name, F func)`
is used to wrap a C++ function to a `v8::Function` value. The V8 function will
be created as anonymous when `nullptr` or `""` string literal is supplied for
its `name`:

```c++
// C++ code
int f(int x) { return x * 2; }

v8::Isolate* isolate = v8::Isolate::GetCurrent();

v8::Local<v8::Function> v8_fun = v8pp::wrap_function(isolate, "f", &f);
isolate->GetCurrentContext()-> Global()->Set(v8pp::to_v8(isolate, "v8_fun"), v8_fun);
```

```js
// JavaScript code
var r = v8_fun(2); // 4
```


## Wrapping C++ objects

### v8pp::module

Class `v8pp::module` in a [`v8pp/module.hpp`](../v8pp/module.hpp) header file
is a thin wrapper that constructs a `v8::ObjectTemplate`. This class also has
a constructor to re-use an existing `v8::ObjectTemplate` instance.

The `v8pp::module` class contains functions that allows binding to V8 for other
`v8pp::modules`, C++ functions, lambdas, global variables, and classes wrapped
with `v8pp::class_` template.

These binding functions return reference to the `v8pp::module` instance to allow
call chaining:

```c++
// C++ code
v8::Isolate* isolate = v8::Isolate::GetCurrent();

int x = 1;
float fun(float a, float b) { return a + b; }

v8pp::module module(isolate), submodule(isolate);

submodule
	.var("x", x)            // bind C++ variable x
	.function("f", &fun)    // bind C++ function
	.function("g", [&x](int y) { return x + y; })
	;

module
	.submodule("sub", submodule) // bind submodule
	.const_("PI", 3.1415926)     // bind constant

// create a  module object and bind it to the global V8 object
isolate->GetCurrentContext()-> Global()->Set(v8pp::to_v8(isolate, "module"), module.new_instance());
```

```js
// JavaScript after bindings above

module.sub.x += 2; // x becomes 3 in C++
var y = module.sub.f(module.sub.x, module.PI); // call C++ function fun(3, 3.1415926)
var z = module.sub.g(1); // call C++ anonymous lambda, returns x+1
```


### v8pp::class_

A class template in [`v8pp/class.hpp`](../v8pp/class.hpp) is used to register
a wrapped C++ class in `v8pp` and to bind the class members into V8.

Allowed `class_` bindings:
  * literla constanst with `const_(name, const_value)`
  * class data members with `var(name, &Class::data_member)`
  * functions with `function(name, &Class::function_member)`
  * static class functions, free functions, lambdas with `function(name, function_or_lambda_ref)`
  * properties with get, and optional set functions or lambdas with `property(name, getter [, setter])`


```c++
struct X
{
	bool b;
	int c;
	X(bool b) : b(b), c(0) {}
};

struct Y : X
{
	X(int v, bool u) : X(u), var(v) {}
	int var;
	int fun() const { return var * 2;}
	int get() const { return var; }
	void set(int x) { var = x; } 
};

static void ext_fun(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	Y* self = v8pp::class_<Y>::unwrap_object(args.GetIsolate(), args.This());
	if (self) args.GetReturnValue().Set(self->b);
	else args.GetReturnValue().Set(args[0]);
}

// bind class X
v8pp::class_<X> X_class(isolate);
X_class
	// specify X constructor signature
	.ctor<bool>()
	// bind class member variable
	.var("b", &X::b)
	// set const property
	.const_("str", "abc")
	// set static property
	.set_static("A", 1)
	// set static const property
	.set_static("B", 42, true)
	;

// Bind class Y
v8pp::class_<Y> Y_class(isolate);
Y_class
	// specify Y constructor signature
	.ctor<int, bool>()
	// inherit bindings from base class
	.inherit<X>()
	// bind member functions
	.function("get", &Y::get)
	.function("set", &Y::set)
	// bind read-only property
	.property("prop", &Y::fun)
	// bind read-write property
	.property("wprop", &Y::get, &Y::set)
	// bind a static function
	.function("ext_fun", &ext_fun)
	;

// Extend existing X bindings
v8pp::class_<X>::extend(isolate).var("c", &X::c);

// set class into the module template
module.class_("X", X_class);
module.class_("Y", Y_class);
```

```javascript
// Use bindings in JavaScript 
var x = new module.X(true);
assert(x.b == true);
assert(x.str == "abc");
// static property
assert(x.A === undefined);
assert(module.X.A === 1);
module.X.A = 2;
assert(module.X.A === 2);
// static const property
assert(x.B === undefined);
assert(module.X.B === 42);
module.X.B = 123;
assert(module.X.B === 42);

var y = new module.Y(10, false);
assert(y.b == false);
assert(y.get() == 10);
assert(y.prop == 20);
y.set(11);
assert(y.get() == 11);
assert(y.wprop == 11);
y.wprop == 12;
assert(y.get() == 12);
assert(y.ext_fun() == y.b);
assert(module.Y.ext_fun(100) == 100);
```
