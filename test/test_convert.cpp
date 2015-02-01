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

	std::vector<int> vector = { 1, 2, 3 };
	test_conv(isolate, vector);
	check("vector to array", v8pp::to_v8(isolate, vector.begin(), vector.end())->IsArray());

	std::list<int> list = { 1, 2, 3 };
	check("list to array", v8pp::to_v8(isolate, list.begin(), list.end())->IsArray());

	std::map<char, int> map = { { 'a', 1 }, { 'b', 2 }, { 'c', 3 } };
	test_conv(isolate, map);
}
