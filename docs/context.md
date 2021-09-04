# Context

File [`v8pp/context.hpp`](../v8pp/context.hpp) contains `v8pp::context` class
that simplifies V8 embedding and maintains custom [plugins](plugins.md)
registry. The `v8pp::context` usage is optional.

Constructor of `v8pp::context` has following optional arguments:
  * `v8::Isolate* isolate = nullptr`: create a new one if `nullptr` with a `v8pp::context::create_isolate()` helper function
  * `v8::ArrayBuffer::Allocator* allocator = nullptr`: use a predefined `malloc/free` allocator if `nullptr`
  * `v8::Local<v8::ObjectTemplate> global = <empty>`: use a custom global template if not empty
  * `add_default_global_methods = true`: add these functions to the `global` JavaScript object: 
    * `require(filename)` - load a plugin module from `filename`
    * `run(filename)` - run a JavaScript code from `file`
  * `enter_context = true`: enter the created context


Context class also supports binding of C++ classes and functions into the
global object similar to [`v8pp::module`](wrapping.md#v8pp::module)

