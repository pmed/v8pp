# Compile time configuration

The library has a set of `#define` directives in
[`v8pp/config.hpp`](../v8pp/config.hpp) header file to customize some options,
mostly for [`plugins`](./plugins.md):

  * `#define V8PP_ISOLATE_DATA_SLOT` - v8::Isolate data slot number internally
    used by v8pp, see documentation for `v8::Isolate::GetNumberOfDataSlots()`,
    `v8::Isolate::SetData()`, and `v8::Isolate::GetData()` functions.

  * `#define V8PP_PLUGIN_INIT_PROC_NAME` - `v8pp` plugin initialization
    procedure name.

  * `#define V8PP_PLUGIN_SUFFIX` - default `v8pp` plugin filename suffix used
    in `require(name)` implementation, `".dll"` on Windows platform, `".so"`
    on others.

  * `#define V8PP_EXPORT` and `#define V8PP_IMPORT` - platfrom-specific
    defines for symbols export and import in loadable plugin modules.

  * `#define V8PP_PLUGIN_INIT(isolate)` - a shortcurt delcaration for plugin
    initialization function.

  * `#define V8PP_HEADER_ONLY 1` - Use header-only implemenation, enabled by default.
