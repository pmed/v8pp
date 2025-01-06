#include "v8pp/call_from_v8.hpp"
#include "v8pp/context.hpp"
#include "v8pp/function.hpp"
#include "v8pp/ptr_traits.hpp"
#include "v8pp/throw_ex.hpp"

#include "test.hpp"

static int x()
{
	return 0;
}
static int y(int a)
{
	return a;
}
static int z(v8::Isolate*, int a)
{
	return a;
}

static void w(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return args.GetReturnValue().Set(args.Length());
}

struct X;
void class_ref(X&);
void class_ptr(X*);
void class_sptr(std::shared_ptr<X>);

using v8pp::detail::call_from_v8_traits;

using v8pp::raw_ptr_traits;
using v8pp::shared_ptr_traits;

template<typename F, size_t Index, typename Traits>
using arg_convert = typename call_from_v8_traits<F>::template arg_converter<
	typename call_from_v8_traits<F>::template arg_type<Index>, Traits>;

// fundamental type converters
static_assert(std::same_as<arg_convert<decltype(y), 0, raw_ptr_traits>, v8pp::convert<int>>);
static_assert(std::same_as<arg_convert<decltype(y), 0, shared_ptr_traits>, v8pp::convert<int>>);

static_assert(std::same_as<arg_convert<decltype(z), 1, raw_ptr_traits>, v8pp::convert<int>>);
static_assert(std::same_as<arg_convert<decltype(z), 1, shared_ptr_traits>, v8pp::convert<int>>);

// cv arg converters
static void s(std::string, std::vector<int>&, std::shared_ptr<int> const&, std::string*, std::string const*) {}

static_assert(std::same_as<arg_convert<decltype(s), 0, raw_ptr_traits>, v8pp::convert<std::string>>);
static_assert(std::same_as<arg_convert<decltype(s), 0, shared_ptr_traits>, v8pp::convert<std::string>>);

static_assert(std::same_as<arg_convert<decltype(s), 1, raw_ptr_traits>, v8pp::convert<std::vector<int>>>);
static_assert(std::same_as<arg_convert<decltype(s), 1, shared_ptr_traits>, v8pp::convert<std::vector<int>>>);

static_assert(std::same_as<arg_convert<decltype(s), 2, raw_ptr_traits>, v8pp::convert<std::shared_ptr<int>>>);
static_assert(std::same_as<arg_convert<decltype(s), 2, shared_ptr_traits>, v8pp::convert<std::shared_ptr<int>>>);

static_assert(std::same_as<arg_convert<decltype(s), 3, raw_ptr_traits>, v8pp::convert<std::string*>>);
static_assert(std::same_as<arg_convert<decltype(s), 3, shared_ptr_traits>, v8pp::convert<std::string*>>);

static_assert(std::same_as<arg_convert<decltype(s), 4, raw_ptr_traits>, v8pp::convert<std::string const*>>);
static_assert(std::same_as<arg_convert<decltype(s), 4, shared_ptr_traits>, v8pp::convert<std::string const*>>);

// fundamental types cv arg converters
static void t(int, char&, bool const&, float*, char const*) {}

static_assert(std::same_as<arg_convert<decltype(t), 0, raw_ptr_traits>, v8pp::convert<int>>);
static_assert(std::same_as<arg_convert<decltype(t), 0, shared_ptr_traits>, v8pp::convert<int>>);

static_assert(std::same_as<arg_convert<decltype(t), 1, raw_ptr_traits>, v8pp::convert<char>>);
static_assert(std::same_as<arg_convert<decltype(t), 1, shared_ptr_traits>, v8pp::convert<char>>);

static_assert(std::same_as<arg_convert<decltype(t), 2, raw_ptr_traits>, v8pp::convert<bool>>);
static_assert(std::same_as<arg_convert<decltype(t), 2, shared_ptr_traits>, v8pp::convert<bool>>);

static_assert(std::same_as<arg_convert<decltype(t), 3, raw_ptr_traits>, v8pp::convert<float*>>);
static_assert(std::same_as<arg_convert<decltype(t), 3, shared_ptr_traits>, v8pp::convert<float*>>);

static_assert(std::same_as<arg_convert<decltype(t), 4, raw_ptr_traits>, v8pp::convert<char const*>>);
static_assert(std::same_as<arg_convert<decltype(t), 4, shared_ptr_traits>, v8pp::convert<char const*>>);

void test_call_from_v8()
{
	v8pp::context context;
	v8::Isolate* isolate = context.isolate();
	v8::HandleScope scope(isolate);

	(void)&s; //context.function("s", s);
	(void)&t; //context.function("t", t);
	context.function("x", x);
	context.function("y", y);
	context.function("z", z);
	context.function("w", w);

	check_eq("x", run_script<int>(context, "x()"), 0);
	check_eq("y", run_script<int>(context, "y(1)"), 1);
	check_eq("z", run_script<int>(context, "z(2)"), 2);
	check_eq("w", run_script<int>(context, "w(2, 'd', true, null)"), 4);
}
