#include "class.hpp"
#include "context.hpp"
#include "throw_ex.hpp"
#include "module.hpp"

#include <fstream>

#if defined(WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#else
#error unsupported platform yet
#endif

namespace v8pp {

namespace detail
{

singleton_registry::singletons singleton_registry::items_;
object_registry::objects object_registry::items_;

}

context* context::get(v8::Handle<v8::Object> obj)
{
	v8::Handle<v8::Value> ctx_val = obj->Get(v8::String::NewSymbol("__context"));

	if ( ctx_val.IsEmpty() || !ctx_val->IsExternal() )
	{
		return nullptr;
	}

	return static_cast<context*>(ctx_val.As<v8::External>()->Value());
}

v8::Handle<v8::Value> context::load_module(const v8::Arguments& args)
{
	v8::HandleScope scope;

	if ( args.Length() != 1 )
	{
		return scope.Close(throw_ex("load_module: incorrect arguments"));
	}

	std::string const name = *v8::String::Utf8Value(args[0]);

	context* ctx = context::get(args.Holder());

	if ( !ctx )
	{
		return scope.Close(throw_ex("load_module: context is set up incorrectly"));
	}

	// check if module is already loaded
	context::dynamic_modules::iterator it = ctx->modules_.find(name);
	if ( it != ctx->modules_.end() )
	{
		return scope.Close(it->second.second);
	}

	boost::filesystem::path filename =  ctx->lib_path_ / name;
	filename.replace_extension(V8PP_PLUGIN_SUFFIX);

#if defined(WIN32)
	UINT const prev_error_mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
	HMODULE dl = LoadLibraryW(filename.native().c_str());
	::SetErrorMode(prev_error_mode);
#elif defined(__linux__)
	void *dl = dlopen(filename.c_str(), RTLD_LAZY);
#else
	#error unsupported platform yet
#endif

	if ( !dl )
	{
		return scope.Close(throw_ex("load_module(" + name + "): could not load shared library "
			+ filename.string()));
	}

#if defined(WIN32)
	void *sym = ::GetProcAddress(dl, BOOST_PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME));
#elif defined(__linux__)
	void *sym = dlsym(dl, BOOST_PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME));
#else
	#error unsupported platform yet
#endif

	if ( !sym )
	{
		return scope.Close(throw_ex("load_module(" + name + "): initialization function "
			BOOST_PP_STRINGIZE(V8PP_PLUGIN_INIT_PROC_NAME) " not found in " + filename.string()));
	}

	typedef v8::Handle<v8::Value> (*module_init_proc)();
	module_init_proc init_proc = reinterpret_cast<module_init_proc>(sym);

	v8::Persistent<v8::Value> value = v8::Persistent<v8::Value>::New(init_proc());
	ctx->modules_.insert(context::dynamic_modules::value_type(name, context::dynamic_module(dl, value)));

	return scope.Close(value);
}

void context::unload_modules()
{
	for (dynamic_modules::iterator it = modules_.begin(), end = modules_.end(); it != end; ++it)
	{
		dynamic_module& mod = it->second;
		mod.second.Dispose();
#if defined(WIN32)
		::FreeLibrary((HMODULE)mod.first);
#elif defined(__linux__)
		dlclose(it->second.first);
#endif
	}
	modules_.clear();
}

v8::Handle<v8::Value> context::run_file(const v8::Arguments& args)
{
	v8::HandleScope scope;

	if ( args.Length() != 1)
	{
		return scope.Close(throw_ex("run_file: incorrect arguments"));
	}

	v8::String::Utf8Value const filename(args[0]);

	context* ctx = context::get(args.Holder());
	if ( !ctx )
	{
		return scope.Close(throw_ex("run_file: context is set up incorrectly"));
	}

	return ctx->run(*filename)? v8::True() : v8::False();
}

context::context(boost::filesystem::path const& lib_path)
	: lib_path_(lib_path)
{
	v8::HandleScope scope;

	v8::Handle<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New();

	global_template->Set(v8::String::NewSymbol("__context"), v8::External::New(this));
	global_template->Set(v8::String::NewSymbol("require"), v8::FunctionTemplate::New(load_module));
	global_template->Set(v8::String::NewSymbol("run"), v8::FunctionTemplate::New(run_file));

	v8::Handle<v8::Context> ctx = v8::Context::New(v8::Isolate::GetCurrent(), nullptr, global_template);
	impl_ = v8::Persistent<v8::Context>::New(ctx);
}

context::~context()
{
	if ( !impl_.IsEmpty())
	{
		unload_modules();
		impl_.Dispose(); impl_.Clear();
	}
}

void context::add(char const *name, module& m)
{
	impl_->Global()->Set(v8::String::NewSymbol(name), m.new_instance());
}

bool context::run(char const *filename)
{
	v8::Context::Scope context_scope(impl_);
	v8::HandleScope scope;
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

	v8::Handle<v8::Script> script = v8::Script::Compile(v8::String::New(source.c_str()), v8::String::New(filename));

	return !script.IsEmpty() && !script->Run().IsEmpty();
}

} // namespace v8pp
