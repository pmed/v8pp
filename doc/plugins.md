# Plugins

The library allows to use a simple loadable plugins. A target platform should support dynamically loadable modules
(i.e. .so or .dll libraries). The target application should contain [`v8pp::context`](./context.md) implementation.

Each plugin exports an initialization procedure `V8PP_PLUGIN_INIT(v8::Isolate* isolate)` which adds
bindings to the V8 `isolate` instance and should return an exported object:

```c++
// in plugin project

V8PP_PLUGIN_INIT(v8::Isolate* isolate)
{
	return v8pp::to_v8(isolate, 42);
}
```

The returned value is a `require()` result in JavaScript:

```js
var plugin_exports = require('plugin');
console.log(plugin_exports); // 42
```

See [`plugins`](../plugins) directory for samples.
