# v8pp

v8pp is a project that allows one to give JavaScript access to C++ classes and methods. The binding library is a header only library that uses template-metaprogramming extensively to make binding as easy as possible for the library user.

This library makes writing node.js modules that take advantage of C++ methods very easy but is also ideal for use in C++ systems where node.js is not required.

## v8pp also provides

* v8pp - a static library to add several global functions (load/require to the v8 JavaScript context. require() is a system for loading plugins from shared libraries.
* test - A binary for running JavaScript files in a context which has v8pp module loading functions provided.

## v8pp module example
```c++
#include <iostream>
#include <v8pp/module.hpp>

namespace console {

v8::Handle<v8::Value> log(v8::Arguments const& args)
{
    for (int i = 0; i < args.Length(); ++i)
    {
        v8::HandleScope handle_scope;
        if (i > 0) std::cout << ' ';
        v8::String::Utf8Value str(args[i]);
        std::cout <<  *str;
    }
    std::cout << std::endl;
    return v8::Undefined();
}

v8::Handle<v8::Value> init()
{
    v8::HandleScope scope;

    v8pp::module m;
    m.set("log", log);

    return scope.Close(m.new_instance());
}

} // namespace console
```

## Turning a v8pp module into a v8pp plugin
```c++
V8PP_PLUGIN_INIT
{
    return console::init();
}
```

## v8pp class binding example
```c++
#include <v8pp/module.hpp>
#include <v8pp/class.hpp>

#include <fstream>
#include <boost/filesystem/operations.hpp>

namespace file {

bool rename(char const* src, char const* dest)
{
    boost::system::error_code err;
    boost::filesystem::rename(src, dest, err);
    return !err;
}

bool mkdir(char const* name)
{
    boost::system::error_code err;
    boost::filesystem::create_directories(name, err);
    return !err;
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

class file_writer : file_base
{
public:
    explicit file_writer(v8::Arguments const& args)
    {
        if ( args.Length() == 1)
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

    void print(v8::Arguments const& args)
    {
        for (int i = 0; i < args.Length(); ++i)
        {
            v8::HandleScope scope;
            if (i > 0) stream_ << ' ';
            v8::String::Utf8Value str(args[i]);
            stream_ << *str;
        }
    }

    void println(v8::Arguments const& args)
    {
        print(args);
        stream_ << std::endl;
    }
};

class file_reader : file_base
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

    v8::Handle<v8::Value> getline()
    {
        if ( stream_.good() && ! stream_.eof())
        {
            std::string line;
            std::getline(stream_, line);
            return v8pp::to_v8(line);
        }
        else
        {
            return v8::Undefined();
        }
    }
};

v8::Handle<v8::Value> init()
{
    v8::HandleScope scope;

    // no_factory argument disallow object creation in JavaScript
    v8pp::class_<file_base, v8pp::no_factory> file_base_class;
    file_base_class
        .set("close", &file_base::close)
        .set("good", &file_base::good)
        .set("is_open", &file_base::is_open)
        .set("eof", &file_base::eof)
        ;

    // file_writer inherits from file_base_class
    // Second template argument is a factory. v8_args_factory passes
    // the v8::Arguments directly to constructor of file_writer
    v8pp::class_<file_writer, v8pp::v8_args_factory> file_writer_class(file_base_class);
    file_writer_class
        .set("open", &file_writer::open)
        .set("print", &file_writer::print)
        .set("println", &file_writer::println)
        ;

    // file_reader also inherits from file_base
    // This factory calls file_reader(char const* ) constructor.
    // It converts v8::Arguments to the appropriate C++ arguments.
    v8pp::class_< file_reader, v8pp::factory<char const*> > file_reader_class(file_base_class);
    file_reader_class
        .set("open", &file_reader::open)
        .set("getln", &file_reader::getline)
        ;

    // Create a module to add classes and functions to and return a
    // new instance of the module to be embedded into the v8 context
    v8pp::module m;
    m.set("rename", rename)
     .set("mkdir", mkdir)
     .set("writer", file_writer_class)
     .set("reader", file_reader_class)
        ;

    return scope.Close(m.new_instance());
}

} // namespace file
```

## Creating a v8 context capable of using require() function
```c++
#include <v8pp/context.hpp>

v8pp::context ctx("path/to/plugins/lib");
// script at location jsFile can now use require() function. An application
// that uses v8pp::context must link against v8pp library.
ctx.run("some_file.js");
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
// v8pp::no_factory avoids creating any constructor for your C++ class from
// JavaScript, useful for classes you only wish to inject.
typedef v8pp::class_<my_class, v8pp::no_factory> my_class_wrapper;
v8::Handle<v8::Value> val = my_class_wrapper::reference_external(&my_class::instance());
// Assuming my_class::instance() returns reference to class
```

## Import externally created C++ class into v8pp.
```c++
// Memory for c++ object will be reclaimed by JavaScript using "delete" when
// JavaScript class is deleted.
typedef v8pp::class_<my_class, v8pp::no_factory> my_class_wrapper;
v8::Handle<v8::Value> val = my_class_wrapper::import_external(new my_class);
```

## v8pp alternatives
* [vu8](https://github.com/tsa/vu8)
* [v8-juice](http://code.google.com/p/v8-juice/)
    * Has a slightly higher syntactic overhead than v8pp.
    * Has been tested on more compilers.

## Dependencies
* Boost 1.37+
* V8
