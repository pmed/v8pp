#ifndef V8PP_CONFIG_HPP_INCLUDED
#define V8PP_CONFIG_HPP_INCLUDED

#include <boost/preprocessor/stringize.hpp>

#define V8PP_MAX_ARG_LIMIT 5

#define V8PP_PLUGIN_LIB_PATH ""

#define V8PP_PLUGIN_INIT_PROC_NAME v8pp_module_init

#if defined(WIN32)
#define V8PP_PLUGIN_SUFFIX ".dll"
#else
#define V8PP_PLUGIN_SUFFIX ".so"
#endif

#if defined(_MSC_VER)
#define PLUGIN_EXPORT_API __declspec(dllexport)
#else
#define PLUGIN_EXPORT_API __attribute__((__visibility__("default")))
#endif

#define V8PP_PLUGIN_INIT extern "C" PLUGIN_EXPORT_API v8::Handle<v8::Value> V8PP_PLUGIN_INIT_PROC_NAME()

#endif // V8PP_CONFIG_HPP_INCLUDED
