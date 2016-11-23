//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <functional>
#include <v8.h>

#include "v8pp/utility.hpp"
#include "test.hpp"

namespace {

template<typename Ret, typename F>
void test_ret(F&&)
{
	using R = typename v8pp::detail::function_traits<F>::return_type;
	static_assert(std::is_same<Ret, R>::value, "wrong return_type");
}

template<typename ArgsTuple, typename F>
void test_args(F&&)
{
	using Args = typename v8pp::detail::function_traits<F>::arguments;
	static_assert(std::is_same<ArgsTuple, Args>::value, "wrong arguments");
}

void x() {}
int y(int, float) { return 1; }

void test_function_traits()
{
	test_ret<void>(x);
	test_args<std::tuple<>>(x);

	test_ret<int>(y);
	test_args<std::tuple<int, float>>(y);

	std::function<bool (int, char, float)> z;
	test_ret<bool>(z);
	test_args<std::tuple<int, char, float>>(z);

	auto lambda = [](int, bool) -> char { return 'z'; };
	test_ret<char>(lambda);
	test_args<std::tuple<int, bool>>(lambda);

	struct X
	{
		float f() const { return 0; }
		void g(int) {}
		int h(float, char) const { return 0; }
		void operator()() const {}
	};

	test_ret<float>(&X::f);
	test_args<std::tuple<X const&>>(&X::f);

	test_ret<void>(&X::g);
	test_args<std::tuple<X&, int>>(&X::g);

	test_ret<int>(&X::h);
	test_args<std::tuple<X const&, float, char>>(&X::h);

	test_ret<void>(X());
	test_args<std::tuple<>>(X());
}

void test_tuple_tail()
{
	using v8pp::detail::tuple_tail;

	static_assert(std::is_same<tuple_tail<std::tuple<int>>::type,
		std::tuple<>>::value, "");
	static_assert(std::is_same<tuple_tail<std::tuple<int, char>>::type,
		std::tuple<char>>::value, "");
	static_assert(std::is_same<tuple_tail<std::tuple<int, char, bool>>::type,
		std::tuple<char, bool>>::value, "");
}

int f() { return 1; }
int g(int x) { return x; }
int h(int x, bool) { return x; }

struct Y
{
	int f() const { return 1; }
};

struct Z
{
	void operator()();
};

void test_apply_tuple()
{
	using v8pp::detail::apply_tuple;
	using v8pp::detail::apply;

	apply_tuple(f, std::make_tuple());
	apply_tuple(g, std::make_tuple(1));
	apply_tuple(h, std::make_tuple(1, true));
	
	check_eq("apply(f)", apply(f), 1);
	check_eq("apply(g)", apply(g, 2), 2);
	check_eq("apply(h)", apply(h, 3, true), 3);
}

void test_is_callable()
{
	using v8pp::detail::is_callable;

	static_assert(is_callable<decltype(f)>::value, "f is callable");
	static_assert(is_callable<decltype(g)>::value, "g is callable");
	static_assert(is_callable<decltype(h)>::value, "h is callable");

	auto lambda = [](){};
	static_assert(is_callable<decltype(lambda)>::value, "lambda is callable");

	static_assert(is_callable<Z>::value, "Z is callable");
	static_assert(!is_callable<decltype(&Y::f)>::value, "Y::f is not callable");

	static_assert(!is_callable<int>::value, "int is not callable");
	static_assert(!is_callable<Y>::value, "Y is not callable");
}

} // unnamed namespace

struct some_struct {};
namespace test { class some_class {}; }

void test_utility()
{
	test_apply_tuple();
	test_is_callable();

	using v8pp::detail::type_id;

	check_eq("type_id", type_id<int>().name(), "int");
	check_eq("type_id", type_id<bool>().name(), "bool");
#ifdef _MSC_VER
	check_eq("type_id", type_id<some_struct>().name(), "struct some_struct");
	check_eq("type_id", type_id<test::some_class>().name(), "class test::some_class");
#else
	check_eq("type_id", type_id<some_struct>().name(), "some_struct");
	check_eq("type_id", type_id<test::some_class>().name(), "test::some_class");
#endif
}
