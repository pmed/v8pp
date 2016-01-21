//
// Copyright (c) 2013-2015 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/convert.hpp"
#include "test.hpp"

#include <list>
#include <vector>
#include <map>

template<typename T>
void test_conv(v8::Isolate* isolate, T value)
{
	v8::Local<v8::Value> v8_value = v8pp::to_v8(isolate, value);
	auto const value2 = v8pp::from_v8<T>(isolate, v8_value);
	check_eq(typeid(T).name(), value2, value);
}

void test_convert()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	test_conv(isolate, 1);
	test_conv(isolate, 2.2);
	test_conv(isolate, true);
	test_conv(isolate, 'a');
	test_conv(isolate, "qaz");

	std::initializer_list<int> const items = { 1, 2, 3 };
	check("initilaizer list to array", v8pp::to_v8(isolate, items)->IsArray());

	std::vector<int> vector = items;
	test_conv(isolate, vector);
	check("vector to array", v8pp::to_v8(isolate, vector.begin(), vector.end())->IsArray());

	std::list<int> list = items;
	check("list to array", v8pp::to_v8(isolate, list.begin(), list.end())->IsArray());

	std::map<char, int> map = { { 'a', 1 }, { 'b', 2 }, { 'c', 3 } };
	test_conv(isolate, map);
}
