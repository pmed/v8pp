#include "v8pp/type_info.hpp"
#include "test.hpp"

struct some_struct {};
namespace test { class some_class {}; }
namespace { using other_class = test::some_class; }

void test_type_info()
{
	using v8pp::detail::type_id;

	check_eq("type_id", type_id<int>().name(), "int");
	check_eq("type_id", type_id<bool>().name(), "bool");
	check_eq("type_id", type_id<some_struct>().name(), "some_struct");
	check_eq("type_id", type_id<test::some_class>().name(), "test::some_class");
	check_eq("type_id", type_id<other_class>().name(), "test::some_class");
}
