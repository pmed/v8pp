#include <functional>

#include "v8pp/utility.hpp"
#include "test.hpp"

#include <map>
#include <unordered_map>

namespace {

template<typename Ret, typename F>
void test_ret(F&&)
{
	using R = typename v8pp::detail::function_traits<F>::return_type;
	static_assert(std::same_as<Ret, R>);
}

template<typename ArgsTuple, typename F>
void test_args(F&&)
{
	using Args = typename v8pp::detail::function_traits<F>::arguments;
	static_assert(std::same_as<ArgsTuple, Args>);
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

		float w = 0;
		const int x = 1;
		volatile char y = 0;
		const volatile bool z = true;
		mutable volatile short zz = 0;
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

	static_assert(std::same_as<tuple_tail<std::tuple<int>>::type, std::tuple<>>);
	static_assert(std::same_as<tuple_tail<std::tuple<int, char>>::type, std::tuple<char>>);
	static_assert(std::same_as<tuple_tail<std::tuple<int, char, bool>>::type, std::tuple<char, bool>>);
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

void test_concepts()
{
	static_assert(v8pp::detail::String<std::string>, "std::string");
	static_assert(!v8pp::detail::Sequence<std::string>, "std::string");
	static_assert(!v8pp::detail::Mapping<std::string>, "std::string");
	static_assert(!v8pp::detail::Array<std::string>, "std::string");
	static_assert(v8pp::detail::HasReserve<std::string>, "std::string");
	static_assert(!v8pp::detail::Tuple<std::string>, "std::string");

	static_assert(v8pp::detail::String<std::string_view>, "std::string_view");
	static_assert(v8pp::detail::String<std::u16string>, "std::u16string");
	static_assert(v8pp::detail::String<std::u16string_view>, "std::u16string_view");
	static_assert(v8pp::detail::String<std::u32string>, "std::u32string");
	static_assert(v8pp::detail::String<std::u32string_view>, "std::u32string_view");
	static_assert(v8pp::detail::String<std::wstring>, "std::wstring");
	static_assert(v8pp::detail::String<std::wstring_view>, "std::wstring_view");
	static_assert(v8pp::detail::String<char const*>, "char const*");
	static_assert(v8pp::detail::String<char16_t const*>, "char16_t const*");
	static_assert(v8pp::detail::String<char32_t const*>, "char32_t const*");
	static_assert(v8pp::detail::String<wchar_t const*>, "wchar_t const*");

	static_assert(!v8pp::detail::String<std::array<int, 1>>, "std::array");
	static_assert(!v8pp::detail::Mapping<std::array<int, 1>>, "std::array");
	static_assert(!v8pp::detail::Sequence<std::array<int, 1>>, "std::array");
	static_assert(!v8pp::detail::Sequence<std::array<int, 1>>, "std::array");
	static_assert(v8pp::detail::Array<std::array<int, 1>>, "std::array");
	static_assert(!v8pp::detail::HasReserve<std::array<int, 1>>, "std::array");
	static_assert(!v8pp::detail::Tuple<std::array<int, 1>>, "std::array");

	static_assert(!v8pp::detail::String<std::vector<char>>, "std::vector");
	static_assert(!v8pp::detail::Mapping<std::vector<char>>, "std::vector");
	static_assert(v8pp::detail::Sequence<std::vector<char>>, "std::vector");
	static_assert(!v8pp::detail::Array<std::vector<char>>, "std::vector");
	static_assert(v8pp::detail::HasReserve<std::vector<char>>, "std::vector");
	static_assert(!v8pp::detail::Tuple<std::vector<char>>, "std::vector");

	static_assert(!v8pp::detail::String<std::deque<int>>, "std::deque");
	static_assert(!v8pp::detail::Mapping<std::deque<int>>, "std::deque");
	static_assert(v8pp::detail::Sequence<std::deque<int>>, "std::deque");
	static_assert(!v8pp::detail::Array<std::deque<int>>, "std::deque");
	static_assert(!v8pp::detail::HasReserve<std::deque<int>>, "std::deque");
	static_assert(!v8pp::detail::Tuple<std::deque<int>>, "std::deque");

	static_assert(!v8pp::detail::String<std::list<bool>>, "std::list");
	static_assert(!v8pp::detail::Mapping<std::list<bool>>, "std::list");
	static_assert(v8pp::detail::Sequence<std::list<bool>>, "std::list");
	static_assert(!v8pp::detail::Array<std::list<bool>>, "std::list");
	static_assert(!v8pp::detail::HasReserve<std::list<bool>>, "std::list");
	static_assert(!v8pp::detail::Tuple<std::list<bool>>, "std::list");

	static_assert(!v8pp::detail::String<std::map<int, float>>, "std::map");
	static_assert(v8pp::detail::Mapping<std::map<int, float>>, "std::map");
	static_assert(!v8pp::detail::Sequence<std::map<int, char>>, "std::map");
	static_assert(!v8pp::detail::Array<std::map<int, char>>, "std::map");
	static_assert(!v8pp::detail::HasReserve<std::map<int, char>>, "std::map");
	static_assert(!v8pp::detail::Tuple<std::map<int, char>>, "std::map");

	static_assert(!v8pp::detail::String<std::multimap<int, char>>, "std::multimap");
	static_assert(v8pp::detail::Mapping<std::multimap<bool, std::string, std::greater<>>>, "std::multimap");
	static_assert(!v8pp::detail::Sequence<std::multimap<int, char>>, "std::multimap");
	static_assert(!v8pp::detail::Array<std::multimap<int, char>>, "std::multimap");
	static_assert(!v8pp::detail::HasReserve<std::multimap<int, char>>, "std::multimap");
	static_assert(!v8pp::detail::Tuple<std::multimap<int, char>>, "std::multimap");

	static_assert(!v8pp::detail::String<std::unordered_map<int, char>>, "std::unordered_map");
	static_assert(v8pp::detail::Mapping<std::unordered_map<std::string, std::string>>, "std::unordered_map");
	static_assert(!v8pp::detail::Sequence<std::unordered_map<int, char>>, "std::unordered_map");
	static_assert(!v8pp::detail::Array<std::unordered_map<int, char>>, "std::unordered_map");
	static_assert(v8pp::detail::HasReserve<std::unordered_map<int, char>>, "std::unordered_map");
	static_assert(!v8pp::detail::Tuple<std::unordered_map<int, char>>, "std::unordered_map");

	static_assert(!v8pp::detail::String<std::unordered_multimap<int, char>>, "std::unordered_multimap");
	static_assert(v8pp::detail::Mapping<std::unordered_multimap<char, std::string>>, "std::unordered_multimap");
	static_assert(!v8pp::detail::Sequence<std::unordered_multimap<int, char>>, "std::unordered_multimap");
	static_assert(!v8pp::detail::Array<std::unordered_multimap<int, char>>, "std::unordered_multimap");
	static_assert(v8pp::detail::HasReserve<std::unordered_multimap<int, char>>, "std::unordered_multimap");
	static_assert(!v8pp::detail::Tuple<std::unordered_multimap<int, char>>, "std::unordered_multimap");

	static_assert(v8pp::detail::Tuple<std::tuple<int, char, float>>, "std::tuple");
	static_assert(v8pp::detail::Tuple<std::tuple<std::tuple<bool>>>, "std::tuple");
	static_assert(v8pp::detail::Tuple<std::pair<int, int>>, "std::tuple");
	static_assert(!v8pp::detail::Tuple<std::array<int, 10>>, "std::tuple");
	static_assert(!v8pp::detail::Tuple<int>, "std::tuple");
	static_assert(!v8pp::detail::Tuple<int>, "std::tuple");
	static_assert(!v8pp::detail::String<std::tuple<int, char>>, "std::tuple");
	static_assert(!v8pp::detail::Mapping<std::tuple<int, char>>, "std::tuple");
	static_assert(!v8pp::detail::Sequence<std::tuple<int, char>>, "std::tuple");
	static_assert(!v8pp::detail::Array<std::tuple<int, char>>, "std::tuple");
	static_assert(!v8pp::detail::HasReserve<std::tuple<int, char>>, "std::tuple");

	static_assert(!v8pp::detail::is_shared_ptr<int>::value, "int");
	static_assert(v8pp::detail::is_shared_ptr<std::shared_ptr<int>>::value, "int");
	static_assert(v8pp::detail::is_shared_ptr<std::shared_ptr<std::vector<char>>>::value, "std::vector<char");
	static_assert(!v8pp::detail::is_shared_ptr<std::string>::value, "std::string");
}

} // unnamed namespace

void test_utility()
{
	test_concepts();
	test_function_traits();
	test_tuple_tail();
	test_is_callable();
}
