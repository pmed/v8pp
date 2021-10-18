#include "v8pp/context.hpp"
#include "v8pp/config.hpp"
#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"
#include "v8pp/module.hpp"
#include "v8pp/class.hpp"
#include "v8pp/throw_ex.hpp"

#include <fstream>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
static char const path_sep = '\\';
#else
#include <dlfcn.h>
static char const path_sep = '/';
#endif

namespace v8pp {

struct context::dynamic_module
{
	void* handle;
	v8::Global<v8::Value> exports;
};

void context::load_module(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Value> result;
	try
	{
		std::string const name = from_v8<std::string>(isolate, args[0], "");
		if (name.empty())
		{
			throw std::runtime_error("load_module: require module name string argument");
		}

		context* ctx = detail::external_data::get<context*>(args.Data());
		context::dynamic_modules::iterator it = ctx->modules_.find(name);

		// check if module is already loaded
		if (it != ctx->modules_.end())
		{
			result = v8::Local<v8::Value>::New(isolate, it->second.exports);
		}
		else
		{
			std::string filename = name;
			if (!ctx->lib_path_.empty())
			{
				filename = ctx->lib_path_ + path_sep + name;
			}
			std::string const suffix = V8PP_PLUGIN_SUFFIX;
			if (filename.size() >= suffix.size()
				&& filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) != 0)
			{
				filename += suffix;
			}

			dynamic_module module;
#if defined(WIN32)
			UINT const prev_error_mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
			module.handle = LoadLibraryA(filename.c_str());
			::SetErrorMode(prev_error_mode);
#else
			module.handle = dlopen(filename.c_str(), RTLD_LAZY);
#endif

			if (!module.handle)
			{
				throw std::runtime_error("load_module(" + name
					+ "): could not load shared library " + filename);
			}
#if defined(WIN32)
			void* sym = ::GetProcAddress((HMODULE)module.handle,
				V8PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME));
#else
			void* sym = dlsym(module.handle, V8PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME));
#endif
			if (!sym)
			{
				throw std::runtime_error("load_module(" + name
					+ "): initialization function "
					V8PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME)
					" not found in " + filename);
			}

			using module_init_proc = v8::Local<v8::Value> (*)(v8::Isolate*);
			module_init_proc init_proc = reinterpret_cast<module_init_proc>(sym);
			result = init_proc(isolate);
			module.exports.Reset(isolate, result);
			ctx->modules_.emplace(name, std::move(module));
		}
	}
	catch (std::exception const& ex)
	{
		result = throw_ex(isolate, ex.what());
	}
	args.GetReturnValue().Set(scope.Escape(result));
}

void context::run_file(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();

	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Value> result;
	try
	{
		std::string const filename = from_v8<std::string>(isolate, args[0], "");
		if (filename.empty())
		{
			throw std::runtime_error("run_file: require filename string argument");
		}

		context* ctx = detail::external_data::get<context*>(args.Data());
		result = to_v8(isolate, ctx->run_file(filename));
	}
	catch (std::exception const& ex)
	{
		result = throw_ex(isolate, ex.what());
	}
	args.GetReturnValue().Set(scope.Escape(result));
}

struct array_buffer_allocator : v8::ArrayBuffer::Allocator
{
	void* Allocate(size_t length)
	{
		return calloc(length, 1);
	}
	void* AllocateUninitialized(size_t length)
	{
		return malloc(length);
	}
	void Free(void* data, size_t length)
	{
		free(data);
		(void)length;
	}
};
static array_buffer_allocator array_buffer_allocator_;

v8::Isolate* context::create_isolate(v8::ArrayBuffer::Allocator* allocator)
{
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = allocator ? allocator : &array_buffer_allocator_;

	return v8::Isolate::New(create_params);
}

context::context(v8::Isolate* isolate, v8::ArrayBuffer::Allocator* allocator,
		bool add_default_global_methods, bool enter_context,
		v8::Local<v8::ObjectTemplate> global)
	: own_isolate_(isolate == nullptr)
	, enter_context_(enter_context)
	, isolate_(isolate ? isolate : create_isolate(allocator))
{
	if (own_isolate_)
	{
		isolate_->Enter();
	}

	v8::HandleScope scope(isolate_);

	if (global.IsEmpty())
	{
		global = v8::ObjectTemplate::New(isolate_);
	}

	if (add_default_global_methods)
	{
		v8::Local<v8::Value> data = detail::external_data::set(isolate_, this);
		global->Set(isolate_, "require",
			v8::FunctionTemplate::New(isolate_, context::load_module, data));
		global->Set(isolate_, "run",
			v8::FunctionTemplate::New(isolate_, context::run_file, data));
	}

	v8::Local<v8::Context> impl = v8::Context::New(isolate_, nullptr, global);
	if (enter_context_)
	{
		impl->Enter();
	}
	impl_.Reset(isolate_, impl);
}

context::context(context&& src) noexcept
	: own_isolate_(std::exchange(src.own_isolate_, false))
	, enter_context_(std::exchange(src.enter_context_, false))
	, isolate_(std::exchange(src.isolate_, nullptr))
	, impl_(std::move(src.impl_))
	, modules_(std::move(src.modules_))
	, lib_path_(std::move(src.lib_path_))
{
}

context& context::operator=(context&& src) noexcept
{
	if (&src != this)
	{
		destroy();

		own_isolate_ = std::exchange(src.own_isolate_, false);
		enter_context_ = std::exchange(src.enter_context_, false);
		isolate_ = std::exchange(src.isolate_, nullptr);
		impl_ = std::move(src.impl_);
		modules_ = std::move(src.modules_);
		lib_path_ = std::move(src.lib_path_);
	}
	return *this;
}

context::~context()
{
	destroy();
}

void context::destroy()
{
	if (isolate_ == nullptr && impl_.IsEmpty())
	{
		// moved out state
		return;
	}

	// remove all class singletons and external data before modules unload
	cleanup(isolate_);

	for (auto& kv : modules_)
	{
		dynamic_module& module = kv.second;
		module.exports.Reset();
		if (module.handle)
		{
#if defined(WIN32)
			::FreeLibrary((HMODULE)module.handle);
#else
			dlclose(module.handle);
#endif
		}
	}
	modules_.clear();

	if (enter_context_)
	{
		to_local(isolate_, impl_)->Exit();
	}
	impl_.Reset();

	if (own_isolate_)
	{
		isolate_->Exit();
		isolate_->Dispose();
	}
	isolate_ = nullptr;
}

context& context::set(string_view name, v8::Local<v8::Value> value)
{
	v8::HandleScope scope(isolate_);
	to_local(isolate_, impl_)->Global()->Set(isolate_->GetCurrentContext(), to_v8(isolate_, name), value).FromJust();
	return *this;
}

context& context::set(string_view name, v8pp::module& m)
{
	return set(name, m.new_instance());
}

v8::Local<v8::Value> context::run_file(std::string const& filename)
{
	std::ifstream stream(filename.c_str());
	if (!stream)
	{
		throw std::runtime_error("could not locate file " + filename);
	}

	std::istreambuf_iterator<char> begin(stream), end;
	return run_script(std::string(begin, end), filename);
}

v8::Local<v8::Value> context::run_script(string_view source, string_view filename)
{
	v8::EscapableHandleScope scope(isolate_);
	v8::Local<v8::Context> context = isolate_->GetCurrentContext();

	v8::ScriptOrigin origin(to_v8(isolate_, filename));
	v8::Local<v8::Script> script;
	bool const is_valid = v8::Script::Compile(context,
		to_v8(isolate_, source), &origin).ToLocal(&script);
	(void)is_valid;

	v8::Local<v8::Value> result;
	if (!script.IsEmpty())
	{
		bool const is_successful = script->Run(context).ToLocal(&result);
		(void)is_successful;
	}
	return scope.Escape(result);
}

} // namespace v8pp
