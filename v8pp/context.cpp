
#include "v8pp/class.hpp"
#include "v8pp/context.hpp"
#include "v8pp/convert.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/module.hpp"

#include <fstream>

#if defined(WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace v8pp {

static const int32_t context_slot = 1;

static context* get_context(std::string const& function_name, v8::Isolate* isolate)
{
	context* ctx = static_cast<context*>(isolate->GetData(context_slot));
	return ctx ? ctx : throw std::runtime_error(function_name + ": context is set up incorrectly");
}

void context::load_module(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Value> result;
	try
	{
		if (args.Length() != 1 || !args[0]->IsString())
		{
			throw std::runtime_error("load_module: require module name string argument");
		}

		std::string const name = *v8::String::Utf8Value(args[0]);

		context* ctx = get_context("load_module", isolate);

		// check if module is already loaded
		context::dynamic_modules::iterator it = ctx->modules_.find(name);
		if (it != ctx->modules_.end())
		{
			result = v8::Local<v8::Value>::New(isolate, it->second.exports);
		}
		else
		{
			boost::filesystem::path filename = ctx->lib_path_ / name;
			filename.replace_extension(V8PP_PLUGIN_SUFFIX);

			dynamic_module module;
#if defined(WIN32)
			UINT const prev_error_mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
			module.handle = LoadLibraryW(filename.native().c_str());
			::SetErrorMode(prev_error_mode);
#else
			module.handle = dlopen(filename.c_str(), RTLD_LAZY);
#endif

			if (!module.handle)
			{
				throw std::runtime_error("load_module(" + name + "): could not load shared library "
					+ filename.string());
			}

#if defined(WIN32)
			void *sym = ::GetProcAddress((HMODULE)module.handle, BOOST_PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME));
#else
			void *sym = dlsym(module.handle, BOOST_PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME));
#endif

			if (!sym)
			{
				throw std::runtime_error("load_module(" + name + "): initialization function "
					BOOST_PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME) " not found in " + filename.string());
			}

			typedef v8::Handle<v8::Value>(*module_init_proc)(v8::Isolate*);
			module_init_proc init_proc = reinterpret_cast<module_init_proc>(sym);
			result = init_proc(isolate);
			module.exports.Reset(isolate, result);

			ctx->modules_.emplace(name, module);
		}
	}
	catch (std::exception const& ex)
	{
		result = throw_ex(isolate, ex.what());
	}
	args.GetReturnValue().Set(scope.Escape(result));
}

void context::unload_modules()
{
	for (dynamic_modules::iterator it = modules_.begin(), end = modules_.end(); it != end; ++it)
	{
		dynamic_module& module = it->second;
		module.exports.Reset();
#if defined(WIN32)
		::FreeLibrary((HMODULE)module.handle);
#else
		dlclose(module.handle);
#endif
	}
	modules_.clear();
}

void context::run_file(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Value> result;
	try
	{
		if (args.Length() != 1 || !args[0]->IsString())
		{
			throw std::runtime_error("run_file: require filename string argument");
		}

		context* ctx = get_context("load_module", isolate);

		v8::String::Utf8Value const filename(args[0]);
		result = ctx->run(*filename) ? v8::True(isolate) : v8::False(isolate);
	}
	catch (std::exception const& ex)
	{
		result = throw_ex(isolate, ex.what());
	}
	args.GetReturnValue().Set(scope.Escape(result));
}

context::context(v8::Isolate* isolate, boost::filesystem::path const& lib_path)
	: isolate_(isolate)
	, lib_path_(lib_path)
{
	if (isolate->GetData(context_slot))
	{
		throw std::runtime_error("Isolate data has already set");
	}
	isolate->SetData(context_slot, this);

	v8::HandleScope scope(isolate);

	v8::Handle<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New(isolate);

	global_template->Set(to_v8(isolate, "require"), v8::FunctionTemplate::New(isolate, load_module));
	global_template->Set(to_v8(isolate, "run"), v8::FunctionTemplate::New(isolate, run_file));

	impl_.Reset(isolate, v8::Context::New(isolate, nullptr, global_template));
}

context::~context()
{
	if (!impl_.IsEmpty())
	{
		unload_modules();
		impl_.Reset();
	}
}

void context::add(char const *name, module& m)
{
	v8::HandleScope scope(isolate_);
	v8::Local<v8::Object> global = isolate_->GetCurrentContext()->Global();
	global->Set(to_v8(isolate_, name), m.new_instance());
}

bool context::run(char const *filename)
{
	v8::HandleScope scope(isolate_);
	v8::Local<v8::Context> ctx = v8::Local<v8::Context>::New(isolate_, impl_);
	v8::Context::Scope context_scope(ctx);
	return run_in_scope(filename);
}

bool context::run_in_scope(char const *filename)
{
	std::ifstream stream(filename);
	if ( !stream )
	{
		throw std::runtime_error(std::string("could not locate file ") + filename);
	}

	std::istreambuf_iterator<char> begin(stream), end;
	std::string const source(begin, end);

	v8::Handle<v8::Script> script = v8::Script::Compile(
		to_v8(isolate_, source.data(), static_cast<int>(source.length())),
		to_v8(isolate_, filename));

	return !script.IsEmpty() && !script->Run().IsEmpty();
}

} // namespace v8pp
