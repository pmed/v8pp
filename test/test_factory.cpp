#include "v8pp/factory.hpp"
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

template<typename T, typename... Args>
void test_(v8::Isolate* isolate, Args&&... args)
{
	T* obj = v8pp::factory<T, v8pp::raw_ptr_traits>::create(isolate, std::forward<Args>(args)...);
	v8pp::factory<T, v8pp::raw_ptr_traits>::destroy(isolate, obj);

	std::shared_ptr<T> sp_obj = v8pp::factory<T, v8pp::shared_ptr_traits>::create(isolate, std::forward<Args>(args)...);
	v8pp::factory<T, v8pp::shared_ptr_traits>::destroy(isolate, sp_obj);
}

void test_factories(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	ctor_types = ctor_count = dtor_count = 0;

	test_<X>(isolate);
	test_<X>(isolate, 1);
	test_<X>(isolate, true, 1.0f);
	test_<X>(isolate, args);

	test_<Y>(isolate, 1);
}

} // unnamed namespace

namespace v8pp {

template<>
struct factory<Y, v8pp::raw_ptr_traits>
{
	static Y* create(v8::Isolate*, int arg)
	{
		++ctor_count;
		return Y::make(arg);
	}

	static void destroy(v8::Isolate*, Y* object)
	{
		Y::done(object);
		++dtor_count;
	}
};

template<>
struct factory<Y, v8pp::shared_ptr_traits>
{
	static std::shared_ptr<Y> create(v8::Isolate*, int arg)
	{
		++ctor_count;
		return std::shared_ptr<Y>(Y::make(arg), Y::done);
	}

	static void destroy(v8::Isolate*, std::shared_ptr<Y> const&)
	{
		// std::shared_ptr deleter will work
		++dtor_count;
	}
};

} // namespace v8pp

void test_factory()
{
	v8pp::context context;

	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);
	v8::Local<v8::Function> fun = v8::Function::New(isolate->GetCurrentContext(), test_factories).ToLocalChecked();

	v8pp::call_v8(isolate, fun, fun);
	check_eq("all ctors called", ctor_types, 0x0F);
	check_eq("ctor count", ctor_count, 10);
	check_eq("dtor count", dtor_count, 10);
}
