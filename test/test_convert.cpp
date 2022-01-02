#include "v8pp/convert.hpp"
#include "test.hpp"

#include <list>
#include <vector>
#include <map>

template<typename T, typename U>
void test_conv(v8::Isolate* isolate, T value, U expected)
{
	auto const obtained = v8pp::from_v8<U>(isolate, v8pp::to_v8(isolate, value));
	check_eq(v8pp::detail::type_id<T>().name(), obtained, expected);

	auto const obtained2 = v8pp::from_v8<T>(isolate, v8pp::to_v8(isolate, expected));
	check_eq(v8pp::detail::type_id<U>().name(), obtained2, value);
}

template<typename T>
void test_conv(v8::Isolate* isolate, T value)
{
	test_conv(isolate, value, value);
}

template<typename Char, size_t N>
void test_string_conv(v8::Isolate* isolate, Char const (&str)[N])
{
	std::basic_string<Char> const str2(str, 2);

	v8pp::basic_string_view<Char> const sv(str);
	v8pp::basic_string_view<Char> const sv2(str, 2);

	test_conv(isolate, str[0]);
	test_conv(isolate, str);
	test_conv(isolate, sv2);

	check_eq("string literal",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, str)), str);
	check_eq("string literal2",
		v8pp::from_v8<Char const*>(isolate, v8pp::to_v8(isolate, str, 2)), str2);
	check("string view",
		v8pp::from_v8<v8pp::basic_string_view<Char>>(isolate, v8pp::to_v8(isolate, sv)) == sv);

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
	bool operator==(person const& other) const
	{
		return name == other.name && age == other.age;
	}

	bool operator!=(person const& other) const
	{
		return !(*this == other);
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
			throw std::runtime_error("expected object");
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

} // namespace v8pp

void test_convert()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	test_conv(isolate, 1);
	test_conv(isolate, 2.2);
	test_conv(isolate, true);

	enum old_enum { A = 1, B = 5, C = -1 };
	test_conv(isolate, B);

	enum class new_enum { X = 'a', Y = 'b', Z = 'c' };
	test_conv(isolate, new_enum::Z);

	test_string_conv(isolate, "qaz");
	test_string_conv(isolate, u"qaz");
#ifdef WIN32
	test_string_conv(isolate, L"qaz");
#endif
	// numeric string
	test_string_conv(isolate, "0");

	const std::vector<int> vector{ 1, 2, 3 };
	test_conv(isolate, vector);
	test_conv(isolate, std::deque<unsigned>{ 1, 2, 3 }, vector);
	test_conv(isolate, std::list<int>{ 1, 2, 3 }, vector);

	const std::array<int, 3> array{ 1, 2, 3 };
	test_conv(isolate, array);
	check_ex<std::runtime_error>("wrong array length", [isolate, &array]()
	{
		v8::Local<v8::Array> arr = v8pp::to_v8(isolate, array);
		v8pp::from_v8<std::array<int, 2>>(isolate, arr);
	});

	test_conv(isolate, std::map<char, int>{ { 'a', 1 }, { 'b', 2 }, { 'c', 3 } });
	test_conv(isolate, std::multimap<int, int>{ { 1, -1 }, { 2, -2 } });
	test_conv(isolate, std::unordered_map<char, std::string>{ { 'x', "1" }, { 'y', "2" } });
	test_conv(isolate, std::unordered_multimap<std::string, int>{ { "0", 0 }, { "a", 1 }, { "b", 2 } });

	check_eq("initializer list to array",
		v8pp::from_v8<std::vector<int>>(isolate, v8pp::to_v8(isolate, { 1, 2, 3 })), vector);

	std::list<int> list = { 1, 2, 3 };
	check_eq("pair of iterators to array",
		v8pp::from_v8<std::vector<int>>(isolate, v8pp::to_v8(isolate, list.begin(), list.end())), vector);

	person p;
	p.name = "Al"; p.age = 33;
	test_conv(isolate, p);

	std::tuple<size_t, bool> const tuple_1{ 2, true };
	test_conv(isolate, tuple_1);

	std::tuple<size_t, bool, std::string> const tuple_2{ 2, true, "test" };
	test_conv(isolate, tuple_2);

	std::tuple<size_t, size_t, size_t> const tuple_3{ 1, 2, 3 };
	test_conv(isolate, tuple_3);

	check_ex<v8pp::invalid_argument>("Tuple", [isolate, &tuple_1]()
	{
		// incorrect number of elements
		v8::Local<v8::Array> tuple_1_ = v8pp::to_v8(isolate, tuple_1);
		v8pp::from_v8<std::tuple<size_t, bool, std::string>>(isolate, tuple_1_);
	});

	check_ex<v8pp::invalid_argument>("String", [isolate, &tuple_1]()
	{
		// wrong types
		v8::Local<v8::Array> tuple_1_ = v8pp::to_v8(isolate, tuple_1);
		v8pp::from_v8<std::tuple<size_t, std::string>>(isolate, tuple_1_);
	});
}
