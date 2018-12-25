//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <functional>

#include "v8pp/utility.hpp"
#include "test.hpp"

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

template<typename Ret, typename Derived, typename F>
void test_ret_derived(F&& f)
{
	using G = typename v8pp::detail::function_traits<F>::template pointer_type<Derived>;
	test_ret<Ret, G>(std::forward<F>(f));
}

template<typename ArgsTuple, typename Derived, typename F>
void test_args_derived(F&& f)
{
	using G = typename v8pp::detail::function_traits<F>::template pointer_type<Derived>;
	test_args<ArgsTuple, G>(std::forward<F>(f));
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
		int h(float, char) volatile { return 0; }
		char i(float) const volatile { return 0; }
		void operator()(int, char&) const volatile {}

		float w;
		const int x = 1;
		volatile char y;
		const volatile bool z = true;
		mutable volatile short zz;
	};

	test_ret<float>(&X::f);
	test_args<std::tuple<X const&>>(&X::f);

	test_ret<void>(&X::g);
	test_args<std::tuple<X&, int>>(&X::g);

	test_ret<int>(&X::h);
	test_args<std::tuple<X volatile&, float, char>>(&X::h);

	test_ret<char>(&X::i);
	test_args<std::tuple<X const volatile&, float>>(&X::i);

	test_ret<void>(X());
	test_args<std::tuple<int, char&>>(X());

	test_ret<float>(&X::w);
	test_args<std::tuple<X&>>(&X::w);

	test_ret<int>(&X::x);
	test_args<std::tuple<X const&>>(&X::x);

	test_ret<char>(&X::y);
	test_args<std::tuple<X volatile&>>(&X::y);

	test_ret<bool>(&X::z);
	test_args<std::tuple<X const volatile&>>(&X::z);

	test_ret<short>(&X::zz);
	test_args<std::tuple<X volatile&>>(&X::zz);

	struct Y : X {};

	test_ret_derived<float, Y>(&Y::f);
	test_args_derived<std::tuple<Y const&>, Y>(&Y::f);

	test_ret_derived<void, Y>(&Y::g);
	test_args_derived<std::tuple<Y&, int>, Y>(&Y::g);

	test_ret_derived<int, Y>(&Y::h);
	test_args_derived<std::tuple<Y volatile&, float, char>, Y>(&Y::h);

	test_ret_derived<char, Y>(&Y::i);
	test_args_derived<std::tuple<Y const volatile&, float>, Y>(&Y::i);

	test_ret_derived<float, Y>(&Y::w);
	test_args_derived<std::tuple<Y&>, Y>(&Y::w);

	test_ret_derived<int, Y>(&Y::x);
	test_args_derived<std::tuple<Y const&>, Y>(&Y::x);

	test_ret_derived<char, Y>(&Y::y);
	test_args_derived<std::tuple<Y volatile&>, Y>(&Y::y);

	test_ret_derived<bool, Y>(&Y::z);
	test_args_derived<std::tuple<Y const volatile&>, Y>(&Y::z);

	test_ret_derived<short, Y>(&Y::zz);
	test_args_derived<std::tuple<Y volatile&>, Y>(&Y::zz);
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

struct some_struct {};
namespace test { class some_class {}; }

void test_utility()
{
	test_function_traits();
	test_tuple_tail();
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
