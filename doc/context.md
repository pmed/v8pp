# Context

File [`v8pp/context.hpp`](../v8pp/context.hpp) contains `v8pp::context` class
that simplifies V8 embedding and maintains custom [plugins](plugins.md)
registry.

Constructor of `v8pp::context` class accepts a pointer to `v8::Isolate`
instance (or creates a new one, if `nullptr` was supplied), creates a
`v8::Context` and initializes the co global object with following functions:

  * require(filename) - load a plugin module
  * run(filename) - run a JavaScript file


Context class also supports binding of C++ classes and functions into the
global object similar to [`v8pp::module`](wrapping.md#v8pp::module)