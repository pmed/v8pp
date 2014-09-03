#ifndef V8PP_MODULE_HPP_INCLUDED
#define V8PP_MODULE_HPP_INCLUDED

#include <v8.h>

#include "v8pp/forward.hpp"
#include "v8pp/property.hpp"

namespace v8pp {

template<typename T>
class class_;

class module
{
public:
	explicit module(v8::Isolate* isolate)
		: isolate_(isolate)
		, obj_(v8::ObjectTemplate::New(isolate))
	{
	}

	// Create a module with existing V8 ObjectTemplate
	explicit module(v8::Isolate* isolate, v8::Handle<v8::ObjectTemplate> obj)
		: isolate_(isolate)
		, obj_(obj)
	{
	}

	v8::Isolate* isolate() { return isolate_; }

	// Set a V8 value
	module& set(char const* name, v8::Handle<v8::Data> value)
	{
		obj_->Set(to_v8(isolate_, name), value);
		return *this;
	}

	// Set C++ class into the module
	template<typename T>
	module& set(char const* name, class_<T>& cl)
	{
		v8::HandleScope scope(isolate_);

		cl.class_function_template()->SetClassName(to_v8(isolate_, name));
		return set(name, cl.js_function_template()->GetFunction());
	}

	// Set any C++ function into the module
	template<typename Function>
	typename boost::enable_if<detail::is_function_pointer<Function>, module&>::type
	set(char const* name, Function f)
	{
		v8::HandleScope scope(isolate_);

		typedef typename detail::function_ptr<Function> FunctionProto;

		v8::FunctionCallback callback = forward_function<FunctionProto>;
		v8::Handle<v8::Value> data = detail::set_external_data(isolate_, f);

		return set(name, v8::FunctionTemplate::New(isolate_, callback, data));
	}

	// Set another module in the module
	module& set(char const* name, module& m)
	{
		return set(name, m.new_instance());
	}

	// Set property
	template<typename GetFunction, typename SetFunction>
	typename boost::enable_if<boost::mpl::and_<
		detail::is_function_pointer<GetFunction>,
		detail::is_function_pointer<SetFunction> >, module&>::type
	set(char const *name, property_<GetFunction, SetFunction> prop)
	{
		v8::HandleScope scope(isolate_);

		v8::AccessorGetterCallback getter = property_<GetFunction, SetFunction>::template get<void>;
		v8::AccessorSetterCallback setter = property_<GetFunction, SetFunction>::template set<void>;
		if (property_<GetFunction, SetFunction>::is_readonly)
		{
			setter = NULL;
		}

		v8::Handle<v8::Value> data = detail::set_external_data(isolate_, prop);
		v8::PropertyAttribute const prop_attrs = v8::PropertyAttribute(v8::DontDelete | (setter? 0 : v8::ReadOnly));

		obj_->SetAccessor(to_v8(isolate_, name), getter, setter, data, v8::DEFAULT, prop_attrs);
		return *this;
	}

	// Set a value convertible to JavaScript as a read-only property
	template<typename Value>
	module& set_const(char const* name, Value value)
	{
		v8::HandleScope scope(isolate_);

		obj_->Set(to_v8(isolate_, name), to_v8(isolate_, value),
			v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	// Create a new module instance, this is a local handle so make it persistent if needs be
	v8::Local<v8::Object> new_instance() { return obj_->NewInstance(); }

private:
	v8::Isolate* isolate_;
	v8::Handle<v8::ObjectTemplate> obj_;
};

} // namespace v8pp

#endif // V8PP_MODULE_HPP_INCLUDED
