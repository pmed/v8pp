#ifndef V8PP_MODULE_HPP_INCLUDED
#define V8PP_MODULE_HPP_INCLUDED

#include <v8.h>

#include "call_from_v8.hpp"
#include "proto.hpp"
#include "to_v8.hpp"
#include "throw_ex.hpp"

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
		obj_->Set(v8::String::NewSymbol(name), v8::FunctionTemplate::New(callback));
		return *this;
	}

	// Set C++ class into the module
	template<typename T, typename F>
	module& set(char const* name, class_<T, F>& cl)
	{
		obj_->Set(v8::String::NewSymbol(name), cl.js_function_template()->GetFunction());
		cl.class_function_template()->SetClassName(v8::String::NewSymbol(name));
		return *this;
	}

	// Set any C++ function into the module
	template<typename Function>
	typename boost::enable_if<detail::is_function_pointer<Function>, module&>::type
	set(char const* name, Function f)
	{
		typedef typename detail::function_ptr<Function> FunctionProto;

		v8::InvocationCallback callback = &forward<FunctionProto>;
		v8::Handle<v8::Value> data = v8::External::New(f);

		obj_->Set(v8::String::NewSymbol(name), v8::FunctionTemplate::New(callback, data));
		return *this;
	}

	// Set any C++ data pointer into the module
	template<typename T>
	typename boost::disable_if<detail::is_function_pointer<T>, module&>::type
	set(char const* name, T t)
	{
		obj_->Set(v8::String::NewSymbol(name), v8::External::New(t));
		return *this;
	}

	// this is a local handle so make it persistent if needs be
	v8::Local<v8::Object> new_instance() { return obj_->NewInstance(); }

private:
// C++ function invoke helpers
	template<typename P>
	static typename P::return_type invoke(const v8::Arguments& args)
	{
		typedef typename P::function_type function_type;
		function_type ptr = detail::get_external_data<function_type>(args.Data());
		return call_from_v8<P>(ptr, args);
	}

	template<typename P>
	static typename boost::disable_if<boost::is_same<void, typename P::return_type>, v8::Handle<v8::Value>>::type
	forward_ret(const v8::Arguments& args)
	{
		return to_v8(invoke<P>(args));
	}

	template<typename P>
	static typename boost::enable_if<boost::is_same<void, typename P::return_type>, v8::Handle<v8::Value>>::type
	forward_ret(const v8::Arguments& args)
	{
		invoke<P>(args);
		return v8::Undefined();
	}

	template<typename P>
	static v8::Handle<v8::Value> forward(const v8::Arguments& args)
	{
		v8::HandleScope scope;
		try
		{
			return scope.Close(forward_ret<P>(args));
		}
		catch (std::exception const& ex)
		{
			return scope.Close(throw_ex(ex.what()));
		}
	}

private:
	v8::Local<v8::ObjectTemplate> obj_;
};

} // namespace v8pp

#endif // V8PP_MODULE_HPP_INCLUDED
