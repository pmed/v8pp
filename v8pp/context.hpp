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
	explicit context(boost::filesystem::path const& lib_path = V8PP_PLUGIN_LIB_PATH);

	~context();

	// Run file, returns false on failure, use v8::TryCatch around it to find out why.
	bool run(char const *filename);

	// Same as run but for use inside a pre-existing handle scope.
	bool run_in_scope(char const *filename);

	// Add module to the context
	void add(char const *name, module& m);

private:
	v8::Persistent<v8::Context> impl_;

	typedef std::pair<void*, v8::Persistent<v8::Value> > dynamic_module;
	typedef boost::unordered_map<std::string, dynamic_module> dynamic_modules;

	static context* get(v8::Handle<v8::Object> obj);
	static v8::Handle<v8::Value> load_module(const v8::Arguments& args);
	void unload_modules();

	static v8::Handle<v8::Value> run_file(const v8::Arguments& args);

	dynamic_modules modules_;
	boost::filesystem::path lib_path_;
};

} // namespace v8pp

#endif // V8PP_CONTEXT_HPP_INCLUDED
