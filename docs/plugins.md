# Plugins

The library allows to use a simple loadable C++ plugins for V8 engine embedded
in a program. A target platform should support dynamically loadable modules
(i.e. `.so` or `.dll` libraries). The target application should contain
[`v8pp::context`](./context.md) implementation from a
[`v8pp/context.cpp`](../v8pp/context.cpp) file.

Each plugin should export an initialization procedure declared as
`V8PP_PLUGIN_INIT(v8::Isolate* isolate)`. This init procedure adds bindings to
the V8 `isolate` instance and should return an exported object:

```c++
// in a plugin shared library
V8PP_PLUGIN_INIT(v8::Isolate* isolate)
{
	return v8pp::to_v8(isolate, 42);
}
```

The exported value retuned is availbale as a `require()` result in JavaScript:

```js
var plugin_exports = require('plugin');
console.log(plugin_exports); // 42
```

See [`plugins`](../plugins) directory for samples.
