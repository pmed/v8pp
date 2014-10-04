#ifndef V8PP_CONTEXT_HPP_INCLUDED
#define V8PP_CONTEXT_HPP_INCLUDED

#include <string>
#include <utility>

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>

#include <v8.h>

#include "v8pp/config.hpp"

namespace v8pp {

class module;

// V8 context
class context
{
public:
	explicit context(v8::Isolate* isolate, boost::filesystem::path const& lib_path = V8PP_PLUGIN_LIB_PATH);

	~context();

	// V8 isolate associated with this context
	v8::Isolate* isolate() { return isolate_; }

	// Run file, returns false on failure, use v8::TryCatch around it to find out why.
	bool run(char const *filename);

	// Same as run but for use inside a pre-existing handle scope.
	bool run_in_scope(char const *filename);

	// Add module to the context
	void add(char const *name, module& m);

private:
	v8::Isolate* isolate_;
	v8::Persistent<v8::Context> impl_;

	struct dynamic_module
	{
		void* handle;
		v8::CopyablePersistentTraits<v8::Value>::CopyablePersistent exports;
	};
	typedef boost::unordered_map<std::string, dynamic_module> dynamic_modules;

	static void load_module(v8::FunctionCallbackInfo<v8::Value> const& args);
	void unload_modules();

	static void run_file(v8::FunctionCallbackInfo<v8::Value> const& args);

	dynamic_modules modules_;
	boost::filesystem::path lib_path_;
};

} // namespace v8pp

#endif // V8PP_CONTEXT_HPP_INCLUDED
