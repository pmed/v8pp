#ifndef V8PP_VERSION_HPP_INCLUDED
#define V8PP_VERSION_HPP_INCLUDED

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

#endif // V8PP_VERSION_HPP_INCLUDED
