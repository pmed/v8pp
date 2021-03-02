//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "v8pp/convert.hpp"
#include "test.hpp"
#include "v8pp/class.hpp"

#include <list>
#include <vector>
#include <map>

template<typename T>
void test_conv(v8::Isolate* isolate, T value)
{
	v8::Local<v8::Value> v8_value = v8pp::to_v8(isolate, value);
	auto const value2 = v8pp::from_v8<T>(isolate, v8_value);
	check_eq(v8pp::detail::type_id<T>().name(), value2, value);
}

template<typename Char, size_t N>
void test_string_conv(v8::Isolate* isolate, Char const (&str)[N])
{
	std::basic_string<Char> const str2(str, 2);

	std::basic_string_view<Char> const sv(str);
	std::basic_string_view<Char> const sv2(str, 2);

	test_conv(isolate, str[0]);
	test_conv(isolate, str);
	test_conv(isolate, sv2);

	check_eq("string literal",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, str)), str);
	check_eq("string literal2",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, str, 2)), str2);
	check_eq("string view",
		v8pp::from_v8<std::basic_string_view<Char>>(isolate, v8pp::to_v8(isolate, sv)), sv);

	Char const* ptr = str;
	check_eq("string pointer",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, ptr)), str);
	check_eq("string pointer2",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, ptr, 2)), str2);

	Char const* empty = str + N - 1; // use last \0 in source string
	check_eq("empty string literal",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, "")), empty);
	check_eq("empty string literal0",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, "", 0)), empty);

	check_eq("empty string pointer",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, empty)), empty);
	check_eq("empty string pointer0",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, empty, 0)), empty);
}

struct person
{
	std::string name;
	int age;

//for test framework
	bool operator!=(person const& other) const
	{
		return name != other.name || age != other.age;
	}

	friend std::ostream& operator<<(std::ostream& os, person const& p)
	{
		return os << "person: " << p.name << " age: " << p.age;
	}
};

namespace v8pp {

template<>
struct convert<person>
{
	using from_type = person;
	using to_type = v8::Local<v8::Object>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsObject();
	}

	static to_type to_v8(v8::Isolate* isolate, person const& p)
	{
		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Object> obj = v8::Object::New(isolate);
		obj->Set(isolate->GetCurrentContext(), v8pp::to_v8(isolate, "name"), v8pp::to_v8(isolate, p.name)).FromJust();
		obj->Set(isolate->GetCurrentContext(), v8pp::to_v8(isolate, "age"), v8pp::to_v8(isolate, p.age)).FromJust();
		/* Simpler after #include <v8pp/object.hpp>
		set_option(isolate, obj, "name", p.name);
		set_option(isolate, obj, "age", p.age);
		*/
		return scope.Escape(obj);
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw std::runtime_error("excpected object");
		}

		v8::HandleScope scope(isolate);
		v8::Local<v8::Object> obj = value.As<v8::Object>();

		person result;
		result.name = v8pp::from_v8<std::string>(isolate,
			obj->Get(isolate->GetCurrentContext(), v8pp::to_v8(isolate, "name")).ToLocalChecked());
		result.age = v8pp::from_v8<int>(isolate,
			obj->Get(isolate->GetCurrentContext(), v8pp::to_v8(isolate, "age")).ToLocalChecked());
		
		/* Simpler after #include <v8pp/object.hpp>
		get_option(isolate, obj, "name", result.name);
		get_option(isolate, obj, "age", result.age);
		*/
		return result;
	}
};

} // v8pp

namespace {
struct U {
    U(int value = 1) : value(value) {}
    int value = 1;
    bool operator==(const U& other) const {
        return value == other.value;
    }
    friend std::ostream& operator<<(std::ostream& os, U const& val)
    {
        return os << val.value;
    }
};
struct U2 {
    U2(double value = 2.) : value(value) {}
    double value = 2.;
    bool operator==(const U2& other) const {
        return value == other.value;
    }
    friend std::ostream& operator<<(std::ostream& os, U2 const& val)
    {
        return os << val.value;
    }
};
struct V {
    V(std::string value = "") : value(value) {}
    std::string value = "test";
    bool operator==(const V& other) const {
        return value == other.value;
    }
    friend std::ostream& operator<<(std::ostream& os, V const& val)
    {
        return os << val.value;
    }
};
struct V2 {
    V2(std::string value = "") : value(value) {}
    std::string value = "test";
    bool operator==(const V2& other) const {
        return value == other.value;
    }
    friend std::ostream& operator<<(std::ostream& os, V2 const& val)
    {
        return os << val.value;
    }
};
} // namespace

template <typename T> struct VariantCheck {};

template <typename ... Ts>
struct VariantCheck<std::variant<Ts...>> {
    VariantCheck(v8::Isolate * isolate) : isolate(isolate) {}
    v8::Isolate * isolate;
    using Variant = std::variant<Ts...>;

    template <typename T>
    static T get(const T& in){
        return in;
    }
    template <typename T>
    static T get(const std::variant<Ts...> &in){
        return std::get<T>(in);
    }

    template <typename T, typename From, typename To, bool get>
    void check(const T &value)
    {
        From values = value;
        auto local = v8pp::convert<From>::to_v8(isolate, values);
        auto back = v8pp::convert<To>::from_v8(isolate, local);
        T returned = VariantCheck::get<T>(back);
        ::check(v8pp::detail::type_id<Variant>().name(), returned == value);
    }

    template <typename T>
    void operator()(T && value)
    {
        using T_ = std::decay_t<T>;
        check<T_, Variant, Variant, true>(value); // variant to variant
        check<T_, Variant, T_, false>(value); // variant to type
        check<T_, T_, Variant, true>(value); // type to variant
    }

    void operator()(std::tuple<Ts...> && values)
    {
        (operator()(std::get<Ts>(values)),...);
    }
};

void test_convert()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	test_conv(isolate, 1);
	test_conv(isolate, 2.2);
	test_conv(isolate, true);

	enum old_enum { A = 1, B = 5, C = - 1 };
	test_conv(isolate, B);

	enum class new_enum { X = 'a', Y = 'b', Z = 'c' };
	test_conv(isolate, new_enum::Z);

	test_string_conv(isolate, "qaz");
	test_string_conv(isolate, u"qaz");
#ifdef WIN32
	test_string_conv(isolate, L"qaz");
#endif

	using int_vector = std::vector<int>;

	int_vector vector = { 1, 2, 3 };
	test_conv(isolate, vector);

	std::map<char, int> map = { { 'a', 1 }, { 'b', 2 }, { 'c', 3 } };
	test_conv(isolate, map);

	std::array<int, 3> array = { { 1, 2, 3 } };
	test_conv(isolate, array);

	check_ex<std::runtime_error>("wrong array length", [isolate, &array]()
	{
		v8::Local<v8::Array> arr = v8pp::to_v8(isolate, array);
		v8pp::from_v8<std::array<int, 2>>(isolate, arr);
	});

	check_eq("initializer list to array",
		v8pp::from_v8<int_vector>(isolate, v8pp::to_v8(isolate,
		{ 1, 2, 3 })), vector);

	std::list<int> list = { 1, 2, 3 };
	check_eq("pair of iterators to array", v8pp::from_v8<int_vector>(isolate,
		v8pp::to_v8(isolate, list.begin(), list.end())), vector);

	person p;
	p.name = "Al"; p.age = 33;
	test_conv(isolate, p);

    // Variant check
    v8pp::class_<U, v8pp::raw_ptr_traits> U_class(isolate);
    U_class.template ctor<>().auto_wrap_objects(true);
    v8pp::class_<U2, v8pp::raw_ptr_traits> U2_class(isolate);
    U2_class.template ctor<>().auto_wrap_objects(true);
    v8pp::class_<V, v8pp::shared_ptr_traits> V_class(isolate);
    V_class.template ctor<>().auto_wrap_objects(true);
    v8pp::class_<V2, v8pp::shared_ptr_traits> V2_class(isolate);
    V2_class.template ctor<>().auto_wrap_objects(true);
    auto V_ = std::make_shared<V>(V{"test" });
    auto V2_ = std::make_shared<V2>(V2{"test2"});
    V_class.reference_external(isolate, V_);
    V2_class.reference_external(isolate, V2_);

    using Variant = std::variant<U, std::shared_ptr<V>, int, std::string, U2, std::shared_ptr<V2>>;
    using ArithmeticVariant = std::variant<bool, float, int32_t>;
    using ArithmeticVariantReversed = std::variant<int32_t, float, bool>;
    using VariantVector = std::variant<std::vector<float>, float, std::string>;

    VariantCheck<Variant> check{isolate};
    check({U{2}, V_, -1, std::string("Hello"), U2{3.}, V2_});

    VariantCheck<ArithmeticVariant> checkArithmetic(context.isolate());
    checkArithmetic({bool{true}, float{5.5f}, int32_t{2}});

    VariantCheck<ArithmeticVariantReversed> checkArithmeticReversed(context.isolate());
    checkArithmeticReversed({int32_t{2}, float{5.5f}, bool{true}});

    VariantCheck<VariantVector> checkVector(context.isolate());
    checkVector({std::vector<float>{1.f, 2.f, 3.f}, float{4.f}, std::string{"testing"}});

    // The order here matters
    using ValueChecking = std::variant<int8_t, uint8_t, int32_t, uint32_t, double>;
    const double largeNumber = static_cast<double>(std::numeric_limits<uint32_t>::max()) + 1.;
    VariantCheck<ValueChecking>{context.isolate()}({-1, 254, std::numeric_limits<int32_t>::min(), std::numeric_limits<uint32_t>::max(), largeNumber});
}
