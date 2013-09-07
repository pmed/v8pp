#ifndef V8PP_MODULE_HPP_INCLUDED
#define V8PP_MODULE_HPP_INCLUDED

#include <v8.h>

#include "forward.hpp"

namespace v8pp {

template<typename T, typename Factory>
class class_;

class module
{
public:
	module() : obj_(v8::ObjectTemplate::New()) {}

	// Set InvocationCallback into the module
	module& set(char const* name, v8::InvocationCallback callback)
	{
		v8::HandleScope scope;

		obj_->Set(v8::String::NewSymbol(name), v8::FunctionTemplate::New(callback));
		return *this;
	}

	// Set C++ class into the module
	template<typename T, typename F>
	module& set(char const* name, class_<T, F>& cl)
	{
		v8::HandleScope scope;

		obj_->Set(v8::String::NewSymbol(name), cl.js_function_template()->GetFunction());
		cl.class_function_template()->SetClassName(v8::String::NewSymbol(name));
		return *this;
	}

	// Set any C++ function into the module
	template<typename Function>
	typename boost::enable_if<detail::is_function_pointer<Function>, module&>::type
	set(char const* name, Function f)
	{
		v8::HandleScope scope;

		typedef typename detail::function_ptr<Function> FunctionProto;

		v8::InvocationCallback callback = forward_function<FunctionProto>;
		v8::Handle<v8::Value> data = detail::set_external_data(f);

		obj_->Set(v8::String::NewSymbol(name), v8::FunctionTemplate::New(callback, data));
		return *this;
	}

	// Set another module in the module
	module& set(char const* name, module& m)
	{
		v8::HandleScope scope;

		obj_->Set(v8::String::NewSymbol(name), m.new_instance());
		return *this;
	}

	// Set value as a read-only property
	template<typename Value>
	module& set_const(char const* name, Value value)
	{
		v8::HandleScope scope;

		obj_->Set(v8::String::NewSymbol(name), v8pp::to_v8(value),
			v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	// this is a local handle so make it persistent if needs be
	v8::Local<v8::Object> new_instance() { return obj_->NewInstance(); }

private:
	v8::Handle<v8::ObjectTemplate> obj_;
};

} // namespace v8pp

#endif // V8PP_MODULE_HPP_INCLUDED
