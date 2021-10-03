#include "v8pp/version.hpp"
#include "v8pp/config.hpp"

namespace v8pp {

V8PP_IMPL char const* version()
{
	return V8PP_VERSION;
}

V8PP_IMPL char const* build_options()
{
#define STR(opt) #opt "=" V8PP_STRINGIZE(opt) " "
	return STR(V8PP_ISOLATE_DATA_SLOT)
		STR(V8PP_USE_STD_STRING_VIEW);
#undef STR
}

} // namespace v8pp
