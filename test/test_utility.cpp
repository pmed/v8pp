#include <functional>

#include "v8pp/utility.hpp"
#include "test.hpp"

#include <map>
#include <unordered_map>

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

	struct Y : X
	{
	};

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

	static_assert(std::is_same<tuple_tail<std::tuple<int>>::type, std::tuple<>>::value, "");
	static_assert(std::is_same<tuple_tail<std::tuple<int, char>>::type, std::tuple<char>>::value, "");
	static_assert(std::is_same<tuple_tail<std::tuple<int, char, bool>>::type, std::tuple<char, bool>>::value, "");
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

void test_type_traits()
{
	static_assert(v8pp::detail::is_string<std::string>::value, "std::string");
	static_assert(!v8pp::detail::is_sequence<std::string>::value, "std::string");
	static_assert(!v8pp::detail::is_mapping<std::string>::value, "std::string");
	static_assert(!v8pp::detail::is_array<std::string>::value, "std::string");

	static_assert(v8pp::detail::is_string<v8pp::string_view>::value, "std::string_view");
	static_assert(v8pp::detail::is_string<std::u16string>::value, "std::u16string");
	static_assert(v8pp::detail::is_string<v8pp::u16string_view>::value, "std::u16string_view");
	//static_assert(v8pp::detail::is_string<std::u32string>::value, "std::u32string");
	//static_assert(v8pp::detail::is_string<v8pp::u32string_view>::value, "std::u32string_view");
	static_assert(v8pp::detail::is_string<std::wstring>::value, "std::wstring");
	static_assert(v8pp::detail::is_string<v8pp::wstring_view>::value, "std::wstring_view");
	static_assert(v8pp::detail::is_string<char const*>::value, "char const*");
	static_assert(v8pp::detail::is_string<char16_t const*>::value, "char16_t const*");
	static_assert(v8pp::detail::is_string<char32_t const*>::value, "char32_t const*");
	static_assert(v8pp::detail::is_string<wchar_t const*>::value, "wchar_t const*");

	static_assert(!v8pp::detail::is_string<std::array<int, 1>>::value, "std::array");
	static_assert(!v8pp::detail::is_mapping<std::array<int, 1>>::value, "std::array");
	static_assert(!v8pp::detail::is_sequence<std::array<int, 1>>::value, "std::array");
	static_assert(!v8pp::detail::is_sequence<std::array<int, 1>>::value, "std::array");
	static_assert(v8pp::detail::is_array<std::array<int, 1>>::value, "std::array");
	static_assert(!v8pp::detail::has_reserve<std::array<int, 1>>::value, "std::array");

	static_assert(!v8pp::detail::is_string<std::vector<char>>::value, "std::vector");
	static_assert(!v8pp::detail::is_mapping<std::vector<char>>::value, "std::vector");
	static_assert(v8pp::detail::is_sequence<std::vector<char>>::value, "std::vector");
	static_assert(!v8pp::detail::is_array<std::vector<char>>::value, "std::vector");
	static_assert(v8pp::detail::has_reserve<std::vector<char>>::value, "std::vector");

	static_assert(!v8pp::detail::is_string<std::deque<int>>::value, "std::deque");
	static_assert(!v8pp::detail::is_mapping<std::deque<int>>::value, "std::deque");
	static_assert(v8pp::detail::is_sequence<std::deque<int>>::value, "std::deque");
	static_assert(!v8pp::detail::is_array<std::deque<int>>::value, "std::deque");
	static_assert(!v8pp::detail::has_reserve<std::deque<int>>::value, "std::deque");

	static_assert(!v8pp::detail::is_string<std::list<bool>>::value, "std::list");
	static_assert(!v8pp::detail::is_mapping<std::list<bool>>::value, "std::list");
	static_assert(v8pp::detail::is_sequence<std::list<bool>>::value, "std::list");
	static_assert(!v8pp::detail::is_array<std::list<bool>>::value, "std::list");
	static_assert(!v8pp::detail::has_reserve<std::list<bool>>::value, "std::list");

	static_assert(!v8pp::detail::is_string<std::tuple<int, char>>::value, "std::tuple");
	static_assert(!v8pp::detail::is_mapping<std::tuple<int, char>>::value, "std::tuple");
	static_assert(!v8pp::detail::is_sequence<std::tuple<int, char>>::value, "std::tuple");
	static_assert(!v8pp::detail::is_array<std::tuple<int, char>>::value, "std::tuple");
	static_assert(v8pp::detail::is_tuple<std::tuple<int, char>>::value, "std::tuple");

	static_assert(!v8pp::detail::is_string<std::map<int, float>>::value, "std::map");
	static_assert(v8pp::detail::is_mapping<std::map<int, float>>::value, "std::map");
	static_assert(!v8pp::detail::is_sequence<std::map<int, char>>::value, "std::map");
	static_assert(!v8pp::detail::is_array<std::map<int, char>>::value, "std::map");

	static_assert(!v8pp::detail::is_string<std::multimap<int, char>>::value, "std::multimap");
	static_assert(v8pp::detail::is_mapping<std::multimap<bool, std::string, std::greater<>>>::value, "std::multimap");
	static_assert(!v8pp::detail::is_sequence<std::multimap<int, char>>::value, "std::multimap");
	static_assert(!v8pp::detail::is_array<std::multimap<int, char>>::value, "std::multimap");

	static_assert(!v8pp::detail::is_string<std::unordered_map<int, char>>::value, "std::unordered_map");
	static_assert(v8pp::detail::is_mapping<std::unordered_map<std::string, std::string>>::value, "std::unordered_map");
	static_assert(!v8pp::detail::is_sequence<std::unordered_map<int, char>>::value, "std::unordered_map");
	static_assert(!v8pp::detail::is_array<std::unordered_map<int, char>>::value, "std::unordered_map");

	static_assert(!v8pp::detail::is_array<std::unordered_multimap<int, char>>::value, "std::unordered_multimap");
	static_assert(v8pp::detail::is_mapping<std::unordered_multimap<char, std::string>>::value, "std::unordered_multimap");
	static_assert(!v8pp::detail::is_sequence<std::unordered_multimap<int, char>>::value, "std::unordered_multimap");
	static_assert(!v8pp::detail::is_array<std::unordered_multimap<int, char>>::value, "std::unordered_multimap");

	static_assert(!v8pp::detail::is_shared_ptr<int>::value, "int");
	static_assert(v8pp::detail::is_shared_ptr<std::shared_ptr<int>>::value, "int");
}

struct some_struct {};
namespace test { class some_class {}; }

void test_utility()
{
	test_type_traits();
	test_function_traits();
	test_tuple_tail();
	test_is_callable();

	using v8pp::detail::type_id;

	check_eq("type_id", type_id<int>().name(), "int");
	check_eq("type_id", type_id<bool>().name(), "bool");
	check_eq("type_id", type_id<some_struct>().name(), "some_struct");
	check_eq("type_id", type_id<test::some_class>().name(), "test::some_class");
}
