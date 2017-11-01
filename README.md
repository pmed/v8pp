[![Travis Build status](https://travis-ci.org/pmed/v8pp.svg)](https://travis-ci.org/pmed/v8pp)
[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/github/pmed/v8pp?svg=true)](https://ci.appveyor.com/project/pmed/v8pp)
[![NPM](https://img.shields.io/npm/v/v8pp.svg)](https://npmjs.com/package/v8pp)
[![Join the chat at https://gitter.im/pmed/v8pp](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/pmed/v8pp?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# v8pp

Header-only library to expose C++ classes and functions into [V8](https://developers.google.com/v8/) to use them in JavaScript code. v8pp uses heavy template metaprogramming and variadic template parameters which requires modern compiler with C++11 support. The library has been tested on:

  * Microsoft Visual C++ 2013 (Windows 7/8)
  * GCC 4.8.1 (Ubuntu 13.10 with Linux kernel 3.11.0)
  * Clang 3.5 (Mac OS X 10.2)

## Binding example

v8pp supports V8 versions after 3.21 with `v8::Isolate` usage in API. There are 2 targets for binding:
 
  * `v8pp::module`, a wrapper class around `v8::ObjectTemplate`
  * `v8pp::class_`, a template class wrapper around `v8::FunctionTemplate`

Both of them require a pointer to `v8::Isolate` instance. They allows to bind from C++ code such items as variables, functions, constants with a function `set(name, item)`:

```c++
v8::Isolate* isolate;

int var;
int get_var() { return var + 1; }
void set_var(int x) { var = x + 1; }

struct X
{
    X(int v, bool u) : var(v) {}
    int var;
    int get() const { return var; }
    voi set(int x) { var = x; } 
};

// bind free variables and functions
v8pp::module mylib(isolate);
mylib
    // set read-only attribute
    .const_("PI", 3.1415)
    // set variable available in JavaScript with name `var`
    .var("var", var)
    // set function get_var as `fun`
    .function("fun", &get_var)
    // set property `prop` with getter get_var() and setter set_var()
    .property("prop", get_var, set_var);

// bind class
v8pp::class_<X> X_class(isolate);
X_class
    // specify X constructor signature
    .ctor<int, bool>()
    // bind variable
    .var("var", &X::var)
    // bind function
    .function("fun", &X::set)
    // bind read-only property
    .property("prop",&X::get);

// set class into the module template
mylib.class_("X", X_class);

// set bindings in global object as `mylib`
isolate->GetCurrentContext()->Global()->Set(
    v8::String::NewFromUtf8(isolate, "mylib"), mylib.new_instance());
```

After that bindings will be available in JavaScript:
```javascript
mylib.var = mylib.PI + mylib.fun();
var x = new mylib.X(1, true);
mylib.prop = x.prop + x.fun();
```

## Node.js and io.js addons

The library is suitable to make [Node.js](http://nodejs.org/) and [io.js](https://iojs.org/) addons. See [addons](docs/addons.md) document.

```c++

void RegisterModule(v8::Handle<v8::Object> exports)
{
    v8pp::module addon(v8::Isolate::GetCurrent());

    // set bindings... 
    addon
        .function("fun", &function)
        .class_("cls", my_class)
        ;

    // set bindings as exports object prototype
    exports->SetPrototype(addon.new_instance());
}
```

## v8pp also provides

* `v8pp` - a static library to add several global functions (load/require to the v8 JavaScript context. `require()` is a system for loading plugins from shared libraries.
* `test` - A binary for running JavaScript files in a context which has v8pp module loading functions provided.

## v8pp module example

```c++
#include <iostream>

#include <v8pp/module.hpp>

namespace console {

void log(v8::FunctionCallbackInfo<v8::Value> const& args)
{
    v8::HandleScope handle_scope(args.GetIsolate());

    for (int i = 0; i < args.Length(); ++i)
    {
        if (i > 0) std::cout << ' ';
        v8::String::Utf8Value str(args[i]);
        std::cout <<  *str;
    }
    std::cout << std::endl;
}

v8::Handle<v8::Value> init(v8::Isolate* isolate)
{
    v8pp::module m(isolate);
    m.function("log", &log);
    return m.new_instance();
}

} // namespace console
```

## Turning a v8pp module into a v8pp plugin

```c++
V8PP_PLUGIN_INIT(v8::Isolate* isolate)
{
    return console::init(isolate);
}
```

## v8pp class binding example

```c++
#include <v8pp/module.hpp>
#include <v8pp/class.hpp>

#include <fstream>

namespace file {

bool rename(char const* src, char const* dest)
{
    return std::rename(src, dest) == 0;
}

class file_base
{
public:
    bool is_open() const { return stream_.is_open(); }
    bool good() const { return stream_.good(); }
    bool eof() const { return stream_.eof(); }
    void close() { stream_.close(); }

protected:
    std::fstream stream_;
};

class file_writer : public file_base
{
public:
    explicit file_writer(v8::FunctionCallbackInfo<v8::Value> const& args)
    {
        if (args.Length() == 1)
        {
            v8::String::Utf8Value str(args[0]);
            open(*str);
        }
    }

    bool open(char const* path)
    {
        stream_.open(path, std::ios_base::out);
        return stream_.good();
    }

    void print(v8::FunctionCallbackInfo<v8::Value> const& args)
    {
        v8::HandleScope scope(args.GetIsolate());

        for (int i = 0; i < args.Length(); ++i)
        {
            if (i > 0) stream_ << ' ';
            v8::String::Utf8Value str(args[i]);
            stream_ << *str;
        }
    }

    void println(v8::FunctionCallbackInfo<v8::Value> const& args)
    {
        print(args);
        stream_ << std::endl;
    }
};

class file_reader : public file_base
{
public:
    explicit file_reader(char const* path)
    {
        open(path);
    }

    bool open(const char* path)
    {
        stream_.open(path, std::ios_base::in);
        return stream_.good();
    }

    v8::Handle<v8::Value> getline(v8::Isolate* isolate)
    {
        if ( stream_.good() && ! stream_.eof())
        {
            std::string line;
            std::getline(stream_, line);
            return v8pp::to_v8(isolate, line);
        }
        else
        {
            return v8::Undefined(isolate);
        }
    }
};

v8::Handle<v8::Value> init(v8::Isolate* isolate)
{
    v8::EscapableHandleScope scope(isolate);

    // file_base binding, no .ctor() specified, object creation disallowed in JavaScript
    v8pp::class_<file_base> file_base_class(isolate);
    file_base_class
        .function("close", &file_base::close)
        .function("good", &file_base::good)
        .function("is_open", &file_base::is_open)
        .function("eof", &file_base::eof)
        ;

    // .ctor<> template arguments declares types of file_writer constructor
    // file_writer inherits from file_base_class
    v8pp::class_<file_writer> file_writer_class(isolate);
    file_writer_class
        .ctor<v8::FunctionCallbackInfo<v8::Value> const&>()
        .inherit<file_base>()
        .function("open", &file_writer::open)
        .function("print", &file_writer::print)
        .function("println", &file_writer::println)
        ;

    // .ctor<> template arguments declares types of file_reader constructor.
    // file_base inherits from file_base_class
    v8pp::class_<file_reader> file_reader_class(isolate);
    file_reader_class
        .ctor<char const*>()
        .inherit<file_base>()
        .function("open", &file_reader::open)
        .function("getln", &file_reader::getline)
        ;

    // Create a module to add classes and functions to and return a
    // new instance of the module to be embedded into the v8 context
    v8pp::module m(isolate);
    m.function("rename", &rename)
     .class_("writer", file_writer_class)
     .class_("reader", file_reader_class)
        ;

    return scope.Escape(m.new_instance());
}

} // namespace file

V8PP_PLUGIN_INIT(v8::Isolate* isolate)
{
    return file::init(isolate);
}
```

## Creating a v8 context capable of using require() function

```c++
#include <v8pp/context.hpp>

v8pp::context context;
context.set_lib_path("path/to/plugins/lib");
// script can now use require() function. An application
// that uses v8pp::context must link against v8pp library.
v8::HandleScope scope(context.isolate());
context.run_file("some_file.js");
```

## Using require() from JavaScript

```javascript
// Load the file module from the class binding example and the
// console module.
var file    = require('file'),
    console = require('console')

var writer = new file.writer("file")
if (writer.is_open()) {
    writer.println("some text")
    writer.close()
    if (! file.rename("file", "newfile"))
        console.log("could not rename file")
}
else console.log("could not open `file'")

console.log("exit")
```

## Create a handle to an externally referenced C++ class.

```c++
// Memory for C++ class will remain when JavaScript object is deleted.
// Useful for classes you only wish to inject.
typedef v8pp::class_<my_class> my_class_wrapper(isolate);
v8::Handle<v8::Value> val = my_class_wrapper::reference_external(&my_class::instance());
// Assuming my_class::instance() returns reference to class
```

## Import externally created C++ class into v8pp.

```c++
// Memory for c++ object will be reclaimed by JavaScript using "delete" when
// JavaScript class is deleted.
typedef v8pp::class_<my_class> my_class_wrapper(isolate);
v8::Handle<v8::Value> val = my_class_wrapper::import_external(new my_class);
```

## Compile-time configuration

The library uses several preprocessor macros, defined in `v8pp/config.hpp` file:

  * `V8PP_ISOLATE_DATA_SLOT` - A v8::Isolate data slot number, used to store v8pp internal data
  * `V8PP_PLUGIN_INIT_PROC_NAME` - Plugin initialization procedure name that should be exported from a v8pp plugin.
  * `V8PP_PLUGIN_SUFFIX` - Plugin filename suffix that would be added if the plugin name used in `require()` doesn't end with it.
  * `V8PP_HEADER_ONLY` - Use header-only implemenation, enabled by default.

## v8pp alternatives

* [nbind](https://github.com/charto/nbind)
* [vu8](https://github.com/tsa/vu8), abandoned
* [v8-juice](http://code.google.com/p/v8-juice/), abandoned
* Script bindng in [cpgf](https://github.com/cpgf/cpgf)
