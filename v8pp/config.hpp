#ifndef V8PP_CONFIG_HPP_INCLUDED
#define V8PP_CONFIG_HPP_INCLUDED

#include <boost/preprocessor/stringize.hpp>

#define V8PP_MAX_ARG_LIMIT 10

#define V8PP_PLUGIN_LIB_PATH "./"

#define V8PP_PLUGIN_INIT_PROC_NAME v8pp_module_init

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

#define V8PP_PLUGIN_INIT extern "C" V8PP_EXPORT v8::Handle<v8::Value> V8PP_PLUGIN_INIT_PROC_NAME()

#if defined(_MSC_VER) && defined(_WINDLL) && !defined(V8PP_USE_GLOBAL_OBJECTS_REGISTRY)
	#define V8PP_USE_GLOBAL_OBJECTS_REGISTRY 1
#endif

#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
	#ifdef V8PP_EXPORT_GLOBAL_OBJECTS_REGISTRY
		#define V8PP_API V8PP_EXPORT
	#else
		#define V8PP_API V8PP_IMPORT
	#endif
#else
	#define V8PP_API
#endif

#endif // V8PP_CONFIG_HPP_INCLUDED
