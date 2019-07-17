# Utilities

`v8pp` contains some utilty functions which maybe helpful to interact with V8.

## Objects

Sometimes it maybe handy to get and set properties from/to a `v8::Object` as
C++ values. Header file [`v8pp/object.hpp`](../v8pp/object.hpp) contains
function templates for these tasks.

### Get an attribute from V8 object

Function `bool v8pp::get_option(v8::Isolate* isolate, v8::Local<v8::Object> object, const* name, T& value)`
allows to get an optional C++ `value` from a V8 `object` by `name`. Symbols
`.` in the `name` delimit sub-object names.

The functon returns `false` if there is no such named value in the `options`.

```c++
v8::Isolate* isolate = v8::Isolate::GetCurrent();
v8::Local<v8::Object> object; // some object, say from JavaScript code

bool flag = true;
int var;
SomeClass some;

bool const flag_exists = v8pp::get_option(isolate, object, "flag", flag);
if (!flag_exists)
{
	assert(flag == true);
}

// get an option from a sub-object, set default value for it before
v8pp::get_option(isolate, object, "some.sub.var", var = 1);

// get complex object (wrapped with v8pp::class_<SomeClass> or specialized
// with v8pp::convert<SomeClass>)
v8pp::get_option(isolate, object, "some", some);
```

### Set an attribute in V8 object

Function `bool v8pp::set_option(v8::Isolate* isolate, v8::Local<v8::Object> object,  const* name, T const& value)`
sets a C++ `value` in a V8 `object` by `name`. Symbols `.` in the `name`
delimit sub-object names.

The function returns `false` if there is no such named value in the `options`.

Function `void v8pp::set_const(v8::Isolate* isolate, v8::Local<v8::Object> object, char const* name, T const& value)`
sets a constant C++ `value` in a V8 `object` by `name`. 

Note this function doesn't support sub-object names.

```c++
v8::Isolate* isolate = v8::Isolate::GetCurrent();
v8::Local<v8::Object> object = v8::Object::New(isolate);

v8pp::set_option(isolate, object, "flag", true);

// set option in sub object
int var = 42;
if (!v8pp::set_option(isolate, object, "some.sub.var", var))
{
	throw std::runtime_error("object some.sub doesn't exists");
}

SomeClass some;
// set complex object wrapped with v8pp::class_<SomeClass>
// or specialized with v8pp::convert<SomeClass>
v8pp::set_option(isolate, object, "some", some);

v8pp::set_const(isolate, object, "PI", 3.1415926);
```


## JSON

V8 library has a `JSON` object in JavaScript, but unfortunatly has no
published C++ API for it.

Header file [`v8pp/json.hpp`](../v8pp/json.hpp) contains a pair of functions
for JSON manipulation.

To stringify a V8 value to JSON use function
`std::string v8pp::json_str(v8::Isolate* isolate, v8::Local<v8::Value> value)`

To parse a JSON string to V8 value use function
`v8::Local<v8::Value> v8pp::json_parse(v8::Isolate* isolate, std::string const& str)`

```c++
v8::Isolate* isolate = v8::Isolate::GetCurrent();

std::string const empty_str = v8pp::json_str(isolate, v8::Local<v8::Value>{}); // empty_str == ""
std::string const int_str = v8pp::json_str(isolate, v8pp::to_v8(isolate, 24)); // int_str == "24"

v8::Local<v8::Value> obj = v8pp::json_parse(isolate, R"({ "x":1, "y":2.2, "z":"abc" })");
v8::Local<v8::Value> arr = v8pp::json_parse(isolate, R"([ 1, 2, 3 ])");
```


## Functions

To call a `v8::Function` with arbitrary number of arguments use
`v8::Local<v8::Value> call_v8(v8::Isolate* isolate, v8::Local<v8::Function> func, v8::Local<v8::Value> recv, Args... args)`
function from [`v8pp/call_v8.hpp`](../v8pp/call_v8.hpp) header file.

This function converts supplied arguments to V8 values using `v8pp::to_v8()`
and invokes the supplied V8 function `func` with `recv` object as `this`.

The function returns result of `func->Call(recv, args...)`.
