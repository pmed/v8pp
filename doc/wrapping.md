# Wrapping C++ code

V8 library uses `v8::ObjectTemplate` to bind C++ objects into JavaScript
and `v8::FunctionTemplate` call C++ functions from JavaScript.

## Wrapping C++ functions

To create a new `v8::FunctionTemplate` for an arbitrary C++ function `func` use a
function template
`v8::Handle<v8::FunctionTemplate> v8pp::wrap_function_template(v8::Isolate* isolate, F func)`

When a function generated from such a function template is being invoked in JavaScript,
all function arguments will be converted from `v8::Value`s to corresponding C++ values,
Then the C++ function `func` will be called and its return type would be converted
back to `v8::Value`. For the C++ function that return `void` a `v8::Undefined` will be returned.

If the wrapped C++ function throws an excption, a `v8::Excpetion::Error` will be returned
to JavaScript code.

There is a function `v8::Handle<v8::Function> v8pp::wrap_function(v8::Isolate* isolate, char const* name, F func)`
to wrap a C++ fuction to a `v8::Function` value. The V8 function will be created anonymous when `nullptr`
or `""` string literal is supplied as the `name`:

```c++
int f(int x) { return x * 2; }

v8::Isolate* isolate;

v8::Local<v8::Function> v8_fun = v8pp::wrap_function(isolate, "f", &f)
isolate->GetCurrentContext()-> Global()->Set(v8pp::to_v8(isolate, "v8_fun"), v8_fun);
```

```js
var r = v8_fun(2); // 4
```


## Wrapping C++ objects

### v8pp::module

There is a `v8pp::module` class in a [`v8pp/module.hpp`](../v8pp/module.hpp) header file
that is essentially a `v8::ObjectTemplate`. It also has a constructor to re-use an existing
`v8::ObjectTemplate` instance.

The `v8pp::module` class contains a number of `set(name, value)` functions that allows binding
to V8 for other `v8pp::modules`, C++ functions, variables, and classes, wrapped with `v8pp::class_`.

These `set` functions return reference to this `v8pp::module` to allow call chaining:

```c++
v8::Isolate* isolate;

int x = 1;
float fun(float a, float b) { return a + b; }

v8pp::module module(isolate), submodule(isolate);

submodule
	.set("x", x)       // bind C++ variable x
	.set("f", &fun)    // bind C++ function
	;

module
	.set("sub", submodule) // bind submodule
	.set_const("PI", 3.1415926)  // bind constant

// create a  module object and bind it to the global V8 object
isolate->GetCurrentContext()-> Global()->Set(v8pp::to_v8(isolate, "module"), module.new_instance());
```

```js
// after bindings above

module.sub.x += 2; // x becomes 3 in C++
var z = module.sub(module.sub.x, module.PI); // call C++ function fun(3, 3.1415926)
```


### v8pp::class_


### v8pp::property

### v8pp::factory