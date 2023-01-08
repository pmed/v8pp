#include "v8pp/version.hpp"
#include "v8pp/config.hpp"

namespace v8pp {

V8PP_IMPL char const* version()
{
	return V8PP_VERSION;
}

V8PP_IMPL unsigned version_major()
{
	return V8PP_VERSION_MAJOR;
}

V8PP_IMPL unsigned version_minor()
{
	return V8PP_VERSION_MINOR;
}

V8PP_IMPL unsigned version_patch()
{
	return V8PP_VERSION_PATCH;
}

V8PP_IMPL char const* build_options()
{
#define STR(opt) #opt "=" V8PP_STRINGIZE(opt) " "
	return ""
#ifdef V8PP_ISOLATE_DATA_SLOT
	STR(V8PP_ISOLATE_DATA_SLOT)
#endif
#ifdef V8PP_HEADER_ONLY
	STR(V8PP_HEADER_ONLY)
#endif
#ifdef V8PP_PLUGIN_INIT_PROC_NAME
	STR(V8PP_PLUGIN_INIT_PROC_NAME)
#endif
#ifdef V8PP_PLUGIN_SUFFIX
	STR(V8PP_PLUGIN_SUFFIX)
#endif
#ifdef V8_COMPRESS_POINTERS
	STR(V8_COMPRESS_POINTERS)
#endif
	;
#undef STR
}

} // namespace v8pp
