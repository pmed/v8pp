#ifndef V8PP_MODULE_HPP_INCLUDED
#define V8PP_MODULE_HPP_INCLUDED

#include <v8.h>

#include "v8pp/forward.hpp"
#include "v8pp/property.hpp"

namespace v8pp {

template<typename T, typename Factory>
class class_;

class module
{
public:
	module() : obj_(v8::ObjectTemplate::New()) {}

	// Create a module with existing V8 ObjectTemplate
	explicit module(v8::Handle<v8::ObjectTemplate> obj) : obj_(obj) {}

	// Set a V8 value
	module& set(char const* name, v8::Handle<v8::Data> value)
	{
		obj_->Set(v8::String::NewSymbol(name), value);
		return *this;
	}

	// Set C++ class into the module
	template<typename T, typename F>
	module& set(char const* name, class_<T, F>& cl)
	{
		v8::HandleScope scope;

		cl.class_function_template()->SetClassName(v8::String::NewSymbol(name));
		return set(name, cl.js_function_template()->GetFunction());
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

		return set(name, v8::FunctionTemplate::New(callback, data));
	}

	// Set another module in the module
	module& set(char const* name, module& m)
	{
		v8::HandleScope scope;

		return set(name, m.new_instance());
	}

	// Set property
	template<typename GetFunction, typename SetFunction>
	typename boost::enable_if<boost::mpl::and_<
		detail::is_function_pointer<GetFunction>,
		detail::is_function_pointer<SetFunction>>, module&>::type
	set(char const *name, property_<GetFunction, SetFunction> prop)
	{
		v8::HandleScope scope;

		v8::AccessorGetter getter = property_<GetFunction, SetFunction>::get;
		v8::AccessorSetter setter = property_<GetFunction, SetFunction>::set;
		if (property_<GetFunction, SetFunction>::is_readonly)
		{
			setter = NULL;
		}

		v8::Handle<v8::Value> data = detail::set_external_data(prop);
		v8::PropertyAttribute const prop_attrs = v8::PropertyAttribute(v8::DontDelete | (setter? 0 : v8::ReadOnly));

		obj_->SetAccessor(v8::String::NewSymbol(name), getter, setter, data, v8::DEFAULT, prop_attrs);
		return *this;
	}

	// Set a value convertible to JavaScript as a read-only property
	template<typename Value>
	module& set_const(char const* name, Value value)
	{
		v8::HandleScope scope;

		obj_->Set(v8::String::NewSymbol(name), v8pp::to_v8(value),
			v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	// Create a new module instance, this is a local handle so make it persistent if needs be
	v8::Local<v8::Object> new_instance() { return obj_->NewInstance(); }

private:
	v8::Handle<v8::ObjectTemplate> obj_;
};

} // namespace v8pp

#endif // V8PP_MODULE_HPP_INCLUDED
