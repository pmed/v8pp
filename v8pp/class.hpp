#ifndef V8PP_CLASS_HPP_INCLUDED
#define V8PP_CLASS_HPP_INCLUDED

#include <boost/mpl/front.hpp>

#include "factory.hpp"
#include "proto.hpp"
#include "call_from_v8.hpp"

namespace v8pp {

namespace detail {

template <typename T, typename F>
struct arg_factory
{
	typedef typename F::template construct<T>  factory_type;

	static typename factory_type::return_type create(v8::Arguments const& args)
	{
		return call_from_v8<factory_type>(&factory_type::create, args);
	}
};

template<typename T>
struct arg_factory<T, v8_args_factory>
{
	static T* create(v8::Arguments const& args)
	{
		return new T(args);
	}
};

} // namespace detail

template <typename M, typename Factory>
class class_singleton_factory
{
public:
	enum { HAS_NULL_FACTORY = false };

	class_singleton_factory()
		: js_func_(v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(&ctor_function)))
	{
	}

	v8::Persistent<v8::FunctionTemplate>& js_function_template_helper()
	{
		return js_func_;
	}

private:
	static v8::Handle<v8::Value> ctor_function(v8::Arguments const& args)
	{
		try
		{
			return M::instance().wrap_object(args);
		}
		catch (std::exception const& ex)
		{
			return throw_ex(ex.what());
		}
	}

	v8::Persistent<v8::FunctionTemplate> js_func_;
};

template<typename M>
class class_singleton_factory<M, no_factory>
{
public:
	enum { HAS_NULL_FACTORY = true };

	v8::Persistent<v8::FunctionTemplate>& js_function_template_helper()
	{
		return static_cast<M&>(*this).class_function_template();
	}
};

template<typename T, typename Factory>
class class_singleton
	: public class_singleton_factory<class_singleton<T, Factory>, Factory>
{
	typedef class class_singleton<T, Factory> self_type;

	class_singleton()
		: func_(v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New()))
	{
		if ( !this->HAS_NULL_FACTORY )
		{
			func_->Inherit(js_function_template());
		}
		func_->InstanceTemplate()->SetInternalFieldCount(1);
	}

	//typedef v8::Handle<v8::Value> (T::*method_callback)(v8::Arguments const& args);

public:
	static self_type& instance()
	{
		static self_type obj;
		return obj;
	}

	v8::Persistent<v8::FunctionTemplate>& class_function_template()
	{
		return func_;
	}

	v8::Persistent<v8::FunctionTemplate>& js_function_template()
	{
		return this->js_function_template_helper();
	}

	v8::Handle<v8::Object> wrap_object(v8::Arguments const& args)
	{
		v8::HandleScope scope;

		T* wrap = detail::arg_factory<T, Factory>::create(args);
		v8::Local<v8::Object> local_obj = func_->GetFunction()->NewInstance();
		v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::New(local_obj);

		obj->SetPointerInInternalField(0, wrap);
		obj.MakeWeak(wrap, on_made_weak);
		return scope.Close(obj);
	}

	// every method is run inside a handle scope
	template<typename P>
	static v8::Handle<v8::Value> forward(v8::Arguments const& args)
	{
		v8::HandleScope scope;

		// this will kill without zero-overhead exception handling
		try
		{
			T* obj = detail::get_native_object_ptr<T>(args.Holder());
			typedef typename P::method_type method_type;
			method_type* pptr = detail::get_external_data<method_type*>(args.Data());
			return scope.Close(forward_ret<P>(obj, *pptr, args));
		}
		catch (std::exception const& ex)
		{
			return throw_ex(ex.what());
		}
	}

private:
	template<typename P>
	struct pass_direct_if : boost::is_same<
		v8::Arguments const&, typename mpl::front<typename P::arguments>::type>
	{
	};

	// invoke passing javascript object argument directly
	template<typename P>
	static typename boost::enable_if<pass_direct_if<P>, typename P::return_type>::type
	invoke(T* obj, typename P::method_type ptr, v8::Arguments const& args)
	{
		return (obj->*ptr)(args);
	}

	template<typename P>
	static typename boost::disable_if<pass_direct_if<P>, typename P::return_type>::type
	invoke(T* obj, typename P::method_type ptr, v8::Arguments const& args)
	{
		return call_from_v8<P>(*obj, ptr, args);
	}

	template<typename P>
	static typename boost::disable_if<detail::is_void_return<P>, v8::Handle<v8::Value>>::type
	forward_ret(T *obj, typename P::method_type ptr, v8::Arguments const& args)
	{
		return to_v8(invoke<P>(obj, ptr, args));
	}

	template<typename P>
	static typename boost::enable_if<detail::is_void_return<P>, v8::Handle<v8::Value>>::type
	forward_ret(T *obj, typename P::method_type ptr, v8::Arguments const& args)
	{
		invoke<P>(obj, ptr, args);
		return v8::Undefined();
	}

	static void on_made_weak(v8::Persistent<v8::Value> object, void* parameter)
	{
		T* obj = static_cast<T*>(parameter);
		delete(obj);
		object.Dispose();
		object.Clear();
	}

private:
	v8::Persistent<v8::FunctionTemplate> func_;

};

// Interface for registering C++ classes with v8
// T = class
// Factory = factory for allocating c++ object
//           by default class_ uses zero-argument constructor
template<typename T, typename Factory = factory<> >
class class_
{
	typedef class_singleton<T, Factory> singleton;
public:
	class_() {}

	// Inhert class from parent
	template<typename U, typename U_Factory>
	explicit class_(class_<U, U_Factory>& parent)
	{
		js_function_template()->Inherit(parent.class_function_template());
	}

	// Set class method with any prototype
	template<typename Method>
	typename boost::enable_if<boost::is_member_function_pointer<Method>, class_&>::type
	set(char const *name, Method m)
	{
		typedef typename detail::mem_function_ptr<T, Method> MethodProto;

		v8::InvocationCallback callback = &singleton::template forward<MethodProto>;
		v8::Handle<v8::Value> data = v8::External::New(new Method(m));

		class_function_template()->PrototypeTemplate()->Set(
			v8::String::NewSymbol(name),
			v8::FunctionTemplate::New(callback, data));
		return *this;
	}

	// create javascript object which references externally created C++
	// class. It will not take ownership of the C++ pointer.
	static v8::Handle<v8::Object> reference_external(T* ext)
	{
		v8::HandleScope scope;
		v8::Local<v8::Object> obj = class_function_template()->GetFunction()->NewInstance();
		obj->SetPointerInInternalField(0, ext);
		return scope.Close(obj);
	}

	// As ReferenceExternal but delete memory for C++ object when javascript
	// object is deleted. You must use "new" to allocate ext.
	static inline v8::Handle<v8::Object> import_external(T* ext)
	{
		v8::HandleScope scope;
		v8::Local<v8::Object> local_obj = class_function_template()->GetFunction()->NewInstance();

		v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::New(local_obj);

		obj->SetPointerInInternalField(0, ext);
		obj.MakeWeak(ext, &singleton::on_made_weak);

		return scope.Close(obj);
	}

	static v8::Persistent<v8::FunctionTemplate>& class_function_template()
	{
		return singleton::instance().class_function_template();
	}

	static v8::Persistent<v8::FunctionTemplate>& js_function_template()
	{
		return singleton::instance().js_function_template();
	}

private:
/*
	// passing v8::Arguments directly but modify return type
	template <class R, R (T::*Ptr)(const v8::Arguments&)>
    inline Class& Set(char const *name) {
        return Set<R(const v8::Arguments&), Ptr>(name);
    }

    // passing v8::Arguments and return v8::Handle<v8::Value> directly
    template <v8::Handle<v8::Value> (T::*Ptr)(const v8::Arguments&)>
    inline Class& Set(char const *name) {
        return Method<v8::Handle<v8::Value>(const v8::Arguments&), Ptr>(name);
    }
*/
private:
	//typedef class_singleton<T, Factory> singleton_type;
	//typedef typename singleton_type::method_callback method_callback;
};

} // namespace v8pp

#endif // V8PP_CLASS_HPP_INCLUDED
