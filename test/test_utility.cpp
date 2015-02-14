#include <functional>
#include <v8.h>

#include "v8pp/utility.hpp"
#include "test.hpp"

namespace {

template<typename Ret, typename F>
void test_ret(F&&)
{
	using namespace v8pp::detail;
	static_assert(std::is_same<Ret, typename function_traits<F>::return_type>::value, "wrong return_type");
}

template<typename ArgsTuple, typename F>
void test_args(F&&)
{
	using namespace v8pp::detail;
	static_assert(std::is_same<ArgsTuple, typename function_traits<F>::arguments>::value, "wrong arguments");
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
	static_assert(std::is_same<v8pp::detail::tuple_tail<std::tuple<int>>::type, std::tuple<>>::value, "");
	static_assert(std::is_same<v8pp::detail::tuple_tail<std::tuple<int, char>>::type, std::tuple<char>>::value, "");
	static_assert(std::is_same<v8pp::detail::tuple_tail<std::tuple<int, char, bool>>::type, std::tuple<char, bool>>::value, "");
}

int f() { return 1; }
int g(int x) { return x; }
int h(int x, bool) { return x; }

struct X
{
	int f() const { return 1; }
};

void test_apply_tuple()
{
	v8pp::detail::apply_tuple(f, std::make_tuple());
	v8pp::detail::apply_tuple(g, std::make_tuple(1));
	v8pp::detail::apply_tuple(h, std::make_tuple(1, true));
	
	check_eq("apply(f)", v8pp::detail::apply(f), 1);
	check_eq("apply(g)", v8pp::detail::apply(g, 2), 2);
	check_eq("apply(h)", v8pp::detail::apply(h, 3, true), 3);
}

} // unnamed namespace

void test_utility()
{
	test_apply_tuple();
}
