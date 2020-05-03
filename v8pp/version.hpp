//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_VERSION_HPP_INCLUDED
#define V8PP_VERSION_HPP_INCLUDED

#include "v8pp/config.hpp"

namespace v8pp {

char const* version();

}

#if V8PP_HEADER_ONLY
#include "v8pp/version.ipp"
#endif

#endif // V8PP_VERSION_HPP_INCLUDED
