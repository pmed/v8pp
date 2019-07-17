# Data conversion

V8 has a set of classes for JavaScript types - `Object`, `Array`, `Number`, 
and others. There is a `v8pp::convert` template to convert fundamental and
user-defined C++ types from and to V8 JavaScript types in a header file
[`v8pp/convert.hpp`](../v8pp/convert.hpp)

Function template `v8pp::to_v8(v8::Isolate*, T const& value)` converts a C++
value to a V8 Value instance:

```c++
v8::Isolate* isolate = v8::Isolate::GetCurrent();

v8::Local<v8::Value>  v8_int = v8pp::to_v8(isolate, 42);
v8::Local<v8::String> v8_str = v8pp::to_v8(isolate, "hello");
```

The opposite function template `v8pp::from_v8<T>(v8::Isolate*, v8::Local<v8::Value> value)`
converts a V8 value to a C++ value of explicitly declared type `T`.

If source V8 value is empty or not convertible to the specified C++ type,
a `std::invalid_argument` exception would be thrown.

An overloaded function `v8pp::from_v8<T>(v8::Isolate*, v8::Local<v8::Value>, T const& default_value)`
converts a V8 value or returns `default_value` on conversion error.

```c++
auto i = v8pp::from_v8<int>(isolate, v8_int); // i == 42
auto s = v8pp::from_v8<std::string>(isolate, v8_str); // s = "hello"

v8pp::from_v8<int>(isolate, v8_str); // throws std::invalid_argument("expected Number")

auto i2 = v8pp::from_v8<int>(isolate, v8_str, -1); // i2 == -1 
```

Currently v8pp allows following conversions:

  * `bool` <-> `v8::Boolean`
  * integral type (`short`, `int`, `long`, and unsigned) <-> `v8::Number`
  * C++ `enum` <--> `v8::Number`
  * floating point (`float`, `double`) <-> `v8::Number`
  * string <-> `v8::String`
  * `std::vector<T>` <-> `v8::Array`
  * `std::map<Key, Value>` <-> `v8::Object`
  * [wrapped](wrapping.md) C++ objects <-> `v8::Object`

**Caution:** JavaScript has no distinct integer an floating types.
It is unsafe to convert integer values greater than 2^53


## Strings

The library supports conversions for UTF-8 encoded `std::string`, UTF-16
encoded `std::wstring`, zero-terminated C strings, and string literals with
optional explicit length supplied:

```c++
v8::Isolate* isolate = v8::Isolate::GetCurrent();

v8::Local<v8::String> v8_str1 = v8pp::to_v8(isolate, std::string("UTF-8 encoded std::string"));
v8::Local<v8::String> v8_str2 = v8pp::to_v8(isolate, "UTF-8 encoded C-string");
v8::Local<v8::String> v8_str3 = v8pp::to_v8(isolate, L"UTF-16 encoded string with optional explicit length", 21);

auto const str1 = v8pp::from_v8<std::string>(isolate, v8_str1);
auto const str2 = v8pp::from_v8<char const*>(isolate, v8_str2); // a `std::string const&` instance, in fact
auto const str3 = v8pp::from_v8<std::wstring>(isolate, v8_str3);
```


## Arrays and Objects

There is a `v8pp::to_v8(v8::Isolate*, InputIterator begin, InputIterator end)`
and `v8pp::to_v8(v8::Isolate* isolate, std::initializer_list<T> const& init)`
function templates to convert a pair of input iterators or initializer list of
elements to V8 `Array`:

```c++
std::list<std::string> container{ "a", "b", "c" };
v8::Local<v8::Array> v8_arr = v8pp::to_v8(isolate, container.begin(), container.end());
v8::Local<v8::Array> v8_arr2 = v8pp::to_v8(isolate, { 1, 2, 3 });

auto arr = v8pp::from_v8<std::vector<std::string>>(isolate, v8_arr);
auto arr2 = v8pp::from_v8<std::array<int, 3>>(isolate, v8_arr2);
```

The library allows conversion between `std::vector<T>` and `v8::Array` if
type `T` is convertible.

The similar is for `std::map<Key, Type>` and `v8::Object` for `Key` and
`Value` types.

```c++
std::vector<int> vector{ 1, 2, 3 };
v8::Local<v8::Array> v8_vector = v8pp::to_v8(isolate, vector);

std::map<std::string, int> map{ { "a", 0 }, { "b", 1 } };
v8::Local<v8::Object> v8_map = v8pp::to_v8(isolate, map);

auto v = v8pp::from_v8<std::vector<int>>(isolate, v8_vector);
auto m = v8pp::from_v8<std::map<std::string, int>>(isolate, v8_map);
```

## Wrapped C++ objects

[Wrapped](wrapping.md) C++ objects can be converted by pointer or by reference:

```c++
class MyClass {};

v8pp::class_<MyClass> bind_MyClass(isolate);

v8::Local<v8::Object> obj = v8pp::class_<MyClass>::create_object(isolate);

MyClass* ptr = v8pp::from_v8<MyClass*>(isolate, obj);
MyClass& ref = v8pp::from_v8<MyClass&>(isolate, obj);

MyClass* none = v8pp::from_v8<MyClass*>(isolate, v8::Null(isolate)); // none == nullptr
MyClass& err = v8pp::from_v8<MyClass&>(isolate, v8::Object::New(isolate)); // throws std::runtime_error("expected C++ wrapped object")

v8::Local<v8::Object> obj2 = v8pp::to_v8(isolate, ptr); // obj == obj2
v8::Local<v8::Object> obj3 = v8pp::to_v8(isolate, ref); // obj == obj3

// unwrapped C++ object converts to empty handle
v8::Local<v8::Object> obj4 = v8pp::to_v8(isolate, new MyClass{}); // obj4.IsEmpty() == true
```


## User-defined types

A `v8pp::convert` template may be specialized to allow conversion from/to
V8 values for user defined types that have not been wrapped with `v8pp::class_`

Such a specialization of `v8pp::convert` template should have following
conversion functions:

```c++
// Generic convertor
template<typename T>
struct convert
{
    // C++ return type for v8pp::from_v8() function
	using from_type = T;

	// V8 return type for v8pp::to_v8() function
	using to_type = v8::Local<v8::Value>;

	// Is V8 value valid to convert from?
	static bool is_valid(v8::Isolate* isolate, v8::Local<v8::Value> value);

	// Convert V8 value to C++
	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value);

	// Convert C++ value to V8
	static to_type to_v8(v8::Isolate* isolate, T const& value);
};
```

Example for a user type:

```c++
struct Vector3
{
	float x, y, z;
};

// Explicit convertor template specialization
template<>
struct v8pp::convert<Vector3>
{
	using from_type = Vector3;
	using to_type = v8::Local<v8::Array>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsArray()
			&& value.As<v8::Array>()->Length() == 3;
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected [x, y, z] array");
		}

		v8::HandleScope scope(isolate);
		v8::Local<v8::Array> arr = value.As<v8::Array>();

		from_type result;
		result.x = v8pp::from_v8<float>(isolate, arr->Get(0));
		result.y = v8pp::from_v8<float>(isolate, arr->Get(1));
		result.z = v8pp::from_v8<float>(isolate, arr->Get(2));

		return result;
	}

	static to_type to_v8(v8::Isolate* isolate, Vector3 const& value)
	{
		v8::EscapableHandleScope scope(isolate);

		v8::Local<v8::Array> arr = v8::Array::New(isolate, 3);
		arr->Set(0, v8pp::to_v8(isolate, value.x));
		arr->Set(1, v8pp::to_v8(isolate, value.y));
		arr->Set(2, v8pp::to_v8(isolate, value.z));

		return scope.Escape(arr);
	}
};

v8::Local<v8::Array> vec3_js = v8pp::to_v8(isolate, Vector3{ 1, 2, 3 });
Vector3 vec3 = v8pp::from_v8<Vector3>(isolate, vec3_js); // == { 1, 2, 3 }

```


User defined class template should also specialize `v8pp::is_wrapped_class`
as `std::false_type` in order to disable conversion as a C++ wrapped
with `v8pp::class_` type:

```c++
template<typename T>
struct Vector3
{
	T x, y, z;
};

template<typename T>
struct v8pp::convert<Vector3<T>>
{
	using from_type = Vector3<T>;
	using to_type = v8::Local<v8::Array>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsArray()
			&& value.As<v8::Array>()->Length() == 3;
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::invalid_argument("expected [x, y, z] array");
		}

		v8::HandleScope scope(isolate);
		v8::Local<v8::Array> arr = value.As<v8::Array>();

		from_type result;
		result.x = v8pp::from_v8<T>(isolate, arr->Get(0));
		result.y = v8pp::from_v8<T>(isolate, arr->Get(1));
		result.z = v8pp::from_v8<T>(isolate, arr->Get(2));

		return result;
	}

	static to_type to_v8(v8::Isolate* isolate, Vector3<T> const& value)
	{
		v8::EscapableHandleScope scope(isolate);

		v8::Local<v8::Array> arr = v8::Array::New(isolate, 3);
		arr->Set(0, v8pp::to_v8(isolate, value.x));
		arr->Set(1, v8pp::to_v8(isolate, value.y));
		arr->Set(2, v8pp::to_v8(isolate, value.z));

		return scope.Escape(arr);
	}
};

template<typename T>
struct v8pp::is_wrapped_class<Vector3<T>> : std::false_type{};

v8::Local<v8::Array> vec3_js = v8pp::to_v8(isolate, Vector3<int>{ 1, 2, 3 });
Vector3<int> vec3 = v8pp::from_v8<Vector3<int>>(isolate, vec3_js); // == { 1, 2, 3 }
```
