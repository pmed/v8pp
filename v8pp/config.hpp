#ifndef V8PP_CONFIG_HPP_INCLUDED
#define V8PP_CONFIG_HPP_INCLUDED

/// v8::Isolate data slot number, used in v8pp
#if !defined(V8PP_ISOLATE_DATA_SLOT)
#define V8PP_ISOLATE_DATA_SLOT 0
#endif

/// v8pp plugin initialization procedure name
#if !defined(V8PP_PLUGIN_INIT_PROC_NAME)
#define V8PP_PLUGIN_INIT_PROC_NAME v8pp_module_init
#endif

/// v8pp plugin filename suffix
#if !defined(V8PP_PLUGIN_SUFFIX)
	#if defined(WIN32)
	#define V8PP_PLUGIN_SUFFIX ".dll"
	#else
	#define V8PP_PLUGIN_SUFFIX ".so"
	#endif
#endif

#if defined(_MSC_VER)
	#define V8PP_EXPORT __declspec(dllexport)
	#define V8PP_IMPORT __declspec(dllimport)
#elif __GNUC__ >= 4
	#define V8PP_EXPORT __attribute__((__visibility__("default")))
	#define V8PP_IMPORT V8PP_EXPORT
#else
	#define V8PP_EXPORT
	#define V8PP_IMPORT
#endif

#define V8PP_PLUGIN_INIT(isolate) extern "C" V8PP_EXPORT v8::Handle<v8::Value> V8PP_PLUGIN_INIT_PROC_NAME(isolate)

#endif // V8PP_CONFIG_HPP_INCLUDED
