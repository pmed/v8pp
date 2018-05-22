//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/context.hpp"

#include "test.hpp"

void test_context()
{
	v8pp::context context;
	int i=999;
	v8::HandleScope scope(context.isolate());
	int const r = context.run_script("42")->Int32Value(context.isolate()->GetCurrentContext()).FromMaybe(i);
	check_eq("run_script", r, 42);
}
