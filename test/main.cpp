#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>
#include <exception>
#include <string>

#include <v8.h>
#include <libplatform/libplatform.h>

#include "v8pp/context.hpp"
#include "v8pp/version.hpp"

void run_tests()
{
	void test_utility();
	void test_context();
	void test_convert();
	void test_throw_ex();
	void test_call_v8();
	void test_call_from_v8();
	void test_function();
	void test_ptr_traits();
	void test_factory();
	void test_module();
	void test_class();
	void test_property();
	void test_object();
	void test_json();

	std::pair<char const*, void (*)()> tests[] =
	{
		{ "test_utility", test_utility },
		{ "test_context", test_context },
		{ "test_convert", test_convert },
		{ "test_throw_ex", test_throw_ex },
		{ "test_function", test_function },
		{ "test_ptr_traits", test_ptr_traits },
		{ "test_call_v8", test_call_v8 },
		{ "test_call_from_v8", test_call_from_v8 },
		{ "test_factory", test_factory },
		{ "test_module", test_module },
		{ "test_class", test_class },
		{ "test_property", test_property },
		{ "test_object", test_object },
		{ "test_json", test_json },
	};

	for (auto const& test : tests)
	{
		std::cout << test.first;
		try
		{
			test.second();
			std::cout << " ok";
		}
		catch (std::exception const& ex)
		{
			std::cerr << " error: " << ex.what();
			exit(EXIT_FAILURE);
		}
		std::cout << std::endl;
	}
}

int main(int argc, char const* argv[])
{
	std::vector<std::string> scripts;
	std::string lib_path;
	bool do_tests = false;

	for (int i = 1; i < argc; ++i)
	{
		std::string const arg = argv[i];
		if (arg == "-h" || arg == "--help")
		{
			std::cout << "Usage: " << argv[0] << " [arguments] [script]\n"
				<< "Arguments:\n"
				<< "  --help,-h           Print this message and exit\n"
				<< "  --version,-v        Print V8 version\n"
				<< "  --lib-path <dir>    Set <dir> for plugins library path\n"
				<< "  --run-tests         Run library tests\n"
				;
			return EXIT_SUCCESS;
		}
		else if (arg == "-v" || arg == "--version")
		{
			std::cout << "V8 version " << v8::V8::GetVersion() << std::endl;
			std::cout << "v8pp version " << v8pp::version() << std::endl;
			std::cout << "v8pp build options " << v8pp::build_options() << std::endl;
		}
		else if (arg == "--lib-path")
		{
			++i;
			if (i < argc) lib_path = argv[i];
		}
		else if (arg == "--run-tests")
		{
			do_tests = true;
		}
		else
		{
			scripts.push_back(arg);
		}
	}

	//v8::V8::InitializeICU();
	v8::V8::InitializeExternalStartupData(argv[0]);
#if V8_MAJOR_VERSION >= 7
	std::unique_ptr<v8::Platform> platform(v8::platform::NewDefaultPlatform());
#else
	std::unique_ptr<v8::Platform> platform(v8::platform::CreateDefaultPlatform());
#endif
	v8::V8::InitializePlatform(platform.get());
	v8::V8::Initialize();

	if (do_tests || scripts.empty())
	{
		run_tests();
	}

	int result = EXIT_SUCCESS;
	try
	{
		v8pp::context context;

		if (!lib_path.empty())
		{
			context.set_lib_path(lib_path);
		}
		for (std::string const& script : scripts)
		{
			v8::HandleScope scope(context.isolate());
			context.run_file(script);
		}
	}
	catch (std::exception const& ex)
	{
		std::cerr << ex.what() << std::endl;
		result = EXIT_FAILURE;
	}

	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();

	return result;
}
