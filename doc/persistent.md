# Persistent handles

The library has declared a `v8pp::persistent_ptr<T>` class template in a
[`v8pp/persistent.hpp`](../v8pp/persistent.hpp) file to store 
[wrapped](wrapping.md) C++ objects in V8 persistent handles.

This class template behavies like a smart pointer and allows to aggregate
wrapped C++ objects inside of other wrapped C++ objects.