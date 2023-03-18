#include "v8pp/context.hpp"
#include "v8pp/call_v8.hpp"
#include "v8pp/ptr_traits.hpp"

#include "test.hpp"

namespace {

int ctor_types = 0;
int ctor_count = 0;
int dtor_count = 0;

struct X
{
	X()
	{
		++ctor_count;
		ctor_types |= 0x01;
	}

	X(int)
	{
		++ctor_count;
		ctor_types |= 0x02;
	}

	X(bool, float)
	{
		++ctor_count;
		ctor_types |= 0x04;
	}

	X(v8::FunctionCallbackInfo<v8::Value> const&)
	{
		++ctor_count;
		ctor_types |= 0x08;
	}

	~X()
	{
		++dtor_count;
	}
};

class Y
{
public:
	static Y* make(int) { return new Y; }
	static void done(Y* y) { delete y; }

private:
	Y() {}
	~Y() {}
};

struct Y_raw_ptr_traits : v8pp::raw_ptr_traits
{
	template<typename T>
	static Y* create(int arg)
	{
		++ctor_count;
		return Y::make(arg);
	}

	template<typename T>
	static void destroy(Y* object)
	{
		Y::done(object);
		++dtor_count;
	}
};

struct Y_shared_ptr_traits : v8pp::shared_ptr_traits
{
	template<typename T>
	static std::shared_ptr<Y> create(int arg)
	{
		++ctor_count;
		return std::shared_ptr<Y>(Y::make(arg), Y::done);
	}

	template<typename T>
	static void destroy(std::shared_ptr<Y> const&)
	{
		// std::shared_ptr deleter will work
		++dtor_count;
	}

	template<typename T>
	static size_t object_size(std::shared_ptr<Y> const&)
	{
		return 100500u;
	}
};

template<typename T, typename Traits, typename... Args>
void test_create_destroy_(Args&&... args)
{
	auto obj = Traits::template create<T>(std::forward<Args>(args)...);
	Traits::template destroy<T>(obj);
}

void test_create_destroy(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	ctor_types = ctor_count = dtor_count = 0;

	test_create_destroy_<X, v8pp::raw_ptr_traits>();
	test_create_destroy_<X, v8pp::raw_ptr_traits>(1);
	test_create_destroy_<X, v8pp::shared_ptr_traits>(true, 1.0f);
	test_create_destroy_<X, v8pp::shared_ptr_traits>(args);

	test_create_destroy_<Y, Y_raw_ptr_traits>(1);
	test_create_destroy_<Y, Y_shared_ptr_traits>(3);
}

void test_raw_ptr_traits()
{
	using traits = v8pp::raw_ptr_traits;

	X x;
	traits::pointer_type ptr = &x;

	static_assert(std::is_same<traits::pointer_type, void*>::value, "raw pointer_type");
	static_assert(std::is_same<traits::const_pointer_type, void const*>::value, "raw const_pointer_type");

	static_assert(std::is_same<traits::object_pointer_type<X>, X*>::value, "raw object_pointer_type");
	static_assert(std::is_same<traits::object_const_pointer_type<X>, X const*>::value, "raw object_const_pointer_type");

	static_assert(std::is_same<traits::object_id, void*>::value, "raw object_id");
	static_assert(std::is_same<traits::convert_ptr<X>, v8pp::convert<X*>>::value, "raw convert_ptr");
	static_assert(std::is_same<traits::convert_ref<X>, v8pp::convert<X&>>::value, "raw convert_ref");

	traits::object_id id = traits::pointer_id(ptr);
	check_eq("raw_ptr_traits::pointer_id", id, &x);
	check_eq("raw_ptr_traits::key", traits::key(id), ptr);
	check_eq("raw_ptr_traits::const_pointer_cast", traits::const_pointer_cast(&x), ptr);
	check_eq("raw_ptr_traits::static_pointer_cast", traits::static_pointer_cast<X>(ptr), &x);
	check_eq("raw_ptr_traits::object_size", traits::object_size<X>(&x), sizeof(X));
}

void test_shared_ptr_traits()
{
	using traits = Y_shared_ptr_traits;

	std::shared_ptr<Y> y(Y::make(1), Y::done);
	traits::pointer_type ptr = y;

	static_assert(std::is_same<traits::pointer_type, std::shared_ptr<void>>::value, "shared pointer_type");
	static_assert(std::is_same<traits::const_pointer_type, std::shared_ptr<void const>>::value, "shared const_pointer_type");

	static_assert(std::is_same<traits::object_pointer_type<Y>, std::shared_ptr<Y>>::value, "shared object_pointer_type");
	static_assert(std::is_same<traits::object_const_pointer_type<Y>, std::shared_ptr<Y const>>::value, "shared object_const_pointer_type");

	static_assert(std::is_same<traits::object_id, void*>::value, "shared object_id");
	static_assert(std::is_same<traits::convert_ptr<Y>, v8pp::convert<std::shared_ptr<Y>>>::value, "shared convert_ptr");
	static_assert(std::is_same<traits::convert_ref<Y>, v8pp::convert<Y, v8pp::ref_from_shared_ptr>>::value, "shared convert_ref");

	traits::object_id id = traits::pointer_id(ptr);
	check_eq("shared_ptr_traits::pointer_id", id, y.get());
	check_eq("shared_ptr_traits::key", traits::key(id), ptr);
	check_eq("shared_ptr_traits::const_pointer_cast", traits::const_pointer_cast(y), ptr);
	check_eq("shared_ptr_traits::static_pointer_cast", traits::static_pointer_cast<Y>(ptr), y);
	check_eq("shared_ptr_traits::object_size", traits::object_size<Y>(y), 100500u);
}

} // unnamed namespace

void test_ptr_traits()
{
	test_raw_ptr_traits();
	test_shared_ptr_traits();

	v8pp::context context;

	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	v8::Local<v8::Function> fun = v8::Function::New(isolate->GetCurrentContext(), test_create_destroy).ToLocalChecked();
	v8pp::call_v8(isolate, fun, fun);
	check_eq("all ctors called", ctor_types, 0x0F);
	check_eq("ctor count", ctor_count, 6);
	check_eq("dtor count", dtor_count, 6);
}
