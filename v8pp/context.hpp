#ifndef V8PP_CONTEXT_HPP_INCLUDED
#define V8PP_CONTEXT_HPP_INCLUDED

#include <string>
#include <map>

#include <v8.h>

#include "v8pp/convert.hpp"

namespace v8pp {

class module;

template<typename T, typename Traits>
class class_;

/// V8 isolate and context wrapper
class context
{
public:
	static v8::Isolate* create_isolate(v8::ArrayBuffer::Allocator* allocator = nullptr);

	struct options
	{
		v8::Isolate* isolate = nullptr;
		v8::ArrayBuffer::Allocator* allocator = nullptr;
		v8::Local<v8::ObjectTemplate> global = {};
		bool add_default_global_methods = true;
		bool enter_context = true;
	};

	/// Create context with optional existing v8::Isolate
	/// and v8::ArrayBuffer::Allocator,
	//  and add default global methods (`require()`, `run()`)
	explicit context(v8::Isolate* isolate = nullptr,
		v8::ArrayBuffer::Allocator* allocator = nullptr,
		bool add_default_global_methods = true,
		bool enter_context = true,
		v8::Local<v8::ObjectTemplate> global = {});

	explicit context(options const& opts)
		: context(opts.isolate, opts.allocator, opts.add_default_global_methods, opts.enter_context, opts.global)
	{
	}

	context(context const&) = delete;
	context& operator=(context const&) = delete;

	context(context&&) noexcept;
	context& operator=(context&&) noexcept;

	~context();

	/// V8 isolate associated with this context
	v8::Isolate* isolate() const { return isolate_; }

	/// V8 context implementation
	v8::Local<v8::Context> impl() const { return to_local(isolate_, impl_); }

	/// Library search path
	std::string const& lib_path() const { return lib_path_; }

	/// Set new library search path
	void set_lib_path(std::string const& lib_path) { lib_path_ = lib_path; }

	/// Run script file, returns script result
	/// or empty handle on failure, use v8::TryCatch around it to find out why.
	/// Must be invoked in a v8::HandleScope
	v8::Local<v8::Value> run_file(std::string const& filename);

	/// The same as run_file but uses string as the script source
	v8::Local<v8::Value> run_script(string_view source, string_view filename = "");

	/// Set a V8 value in the context global object with specified name
	context& set(string_view name, v8::Local<v8::Value> value);

	/// Set module to the context global object
	context& set(string_view name, v8pp::module& m);

	/// Set class to the context global object
	template<typename T, typename Traits>
	context& set(string_view name, v8pp::class_<T, Traits>& cl)
	{
		v8::HandleScope scope(isolate_);
		cl.class_function_template()->SetClassName(v8pp::to_v8(isolate_, name));
		return set(name, cl.js_function_template()->GetFunction(isolate_->GetCurrentContext()).ToLocalChecked());
	}

private:
	void destroy();

	bool own_isolate_;
	bool enter_context_;
	v8::Isolate* isolate_;
	v8::Global<v8::Context> impl_;

	struct dynamic_module;
	using dynamic_modules = std::map<std::string, dynamic_module>;

	static void load_module(v8::FunctionCallbackInfo<v8::Value> const& args);
	static void run_file(v8::FunctionCallbackInfo<v8::Value> const& args);

	dynamic_modules modules_;
	std::string lib_path_;
};

} // namespace v8pp

#endif // V8PP_CONTEXT_HPP_INCLUDED
