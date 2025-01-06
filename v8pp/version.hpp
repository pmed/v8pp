#pragma once

#include "v8pp/config.hpp"

namespace v8pp {

char const* version();

unsigned version_major();
unsigned version_minor();
unsigned version_patch();

char const* build_options();

}

#if V8PP_HEADER_ONLY
#include "v8pp/version.ipp"
#endif
