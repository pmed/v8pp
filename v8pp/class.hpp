#ifndef V8PP_CLASS_HPP_INCLUDED
#define V8PP_CLASS_HPP_INCLUDED

#include <boost/mpl/front.hpp>

#include "factory.hpp"
#include "proto.hpp"
#include "call_from_v8.hpp"
#include "to_v8.hpp"
#include "throw_ex.hpp"

namespace v8pp {

template<typename T, typename Factory>
class class_;

namespace detail {

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
	friend class class_<T, Factory>;

	typedef class class_singleton<T, Factory> self_type;

	template <typename C, typename F>
	struct arg_factory
	{
		typedef typename F::template construct<C> factory_type;

		static typename factory_type::return_type create(v8::Arguments const& args)
		{
			return call_from_v8<factory_type>(&factory_type::create, args);
		}
	};

	template<typename C>
	struct arg_factory<C, v8_args_factory>
	{
		static C* create(v8::Arguments const& args)
		{
			return new C(args);
		}
	};

	class_singleton()
		: func_(v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New()))
	{
		if ( !this->HAS_NULL_FACTORY )
		{
			func_->Inherit(js_function_template());
		}
		func_->InstanceTemplate()->SetInternalFieldCount(1);
	}

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

		T* wrap = arg_factory<T, Factory>::create(args);
		v8::Local<v8::Object> local_obj = func_->GetFunction()->NewInstance();
		v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::New(local_obj);

		obj->SetAlignedPointerInInternalField(0, wrap);
		detail::native_object_registry::add(wrap, obj);
		obj.MakeWeak(wrap, on_made_weak);
		return scope.Close(obj);
	}

private:
	// every method is run inside a handle scope
	template<typename P>
	static v8::Handle<v8::Value> forward(v8::Arguments const& args)
	{
		v8::HandleScope scope;

		// this will kill without zero-overhead exception handling
		try
		{
			T* obj = detail::get_object_field<T*>(args.Holder());
			typedef typename P::method_type method_type;
			method_type* pptr = detail::get_external_data<method_type*>(args.Data());
			return scope.Close(forward_ret<P>(obj, *pptr, args));
		}
		catch (std::exception const& ex)
		{
			return throw_ex(ex.what());
		}
	}

	template<typename P>
	struct pass_direct_if : boost::is_same<
		v8::Arguments const&, typename boost::mpl::front<typename P::arguments>::type>
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

	static void on_made_weak(v8::Isolate*, v8::Persistent<v8::Object>* handle, T* obj)
	{
		detail::native_object_registry::remove(obj);
		delete obj;
	}

private:
	v8::Persistent<v8::FunctionTemplate> func_;
};

} // namespace detail

// Interface for registering C++ classes with v8
// T = class
// Factory = factory for allocating c++ object
//           by default class_ uses zero-argument constructor
template<typename T, typename Factory = factory<> >
class class_
{
	typedef detail::class_singleton<T, Factory> singleton;
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
		v8::HandleScope scope;

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
		obj->SetAlignedPointerInInternalField(0, ext);
		return scope.Close(obj);
	}

	// As ReferenceExternal but delete memory for C++ object when javascript
	// object is deleted. You must use "new" to allocate ext.
	static inline v8::Handle<v8::Object> import_external(T* ext)
	{
		v8::HandleScope scope;

		v8::Local<v8::Object> local_obj = class_function_template()->GetFunction()->NewInstance();
		v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::New(local_obj);

		obj->SetAlignedPointerInInternalField(0, ext);
		obj.MakeWeak(ext, &singleton::on_made_weak);

		return scope.Close(obj);
	}

	// Create a new wrapped JavaScript object in C++ code, return its native pointer
	static T* create(v8::Arguments const& args, v8::Handle<v8::Object>* result_handle = nullptr)
	{
		v8::Handle<v8::Object> object = wrap_object(args);
		if ( result_handle )
		{
			*result_handle = object;
		}
		return reinterpret_cast<T*>(object->GetAlignedPointerFromInternalField(0));
	}

	static T* create(int argc, v8::Handle<v8::Value>* argv, v8::Handle<v8::Object>* result_handle = nullptr)
	{
		v8::Handle<v8::Object> object = wrap_object(argc, argv);
		if ( result_handle )
		{
			*result_handle = object;
		}
		return reinterpret_cast<T*>(object->GetAlignedPointerFromInternalField(0));
	}

	// Destroy JavaScript object from C++ code
	static void destroy(v8::Handle<v8::Object> object)
	{
		T* native = from_v8<T*>(object);
		if ( native )
		{
			singleton::instance().on_made_weak(nullptr, nullptr, native);
		}
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
	// Create wrapped object with array of arguments
	// Throws runtime_error  if arguments doesn't identical to the Factory signature
	static v8::Handle<v8::Object> wrap_object(int argc, v8::Handle<v8::Value>* argv)
	{
		v8::HandleScope scope;

		// Emulate local static function to invoke
		// class_singleton::wrap_object(args) from JavaScript
		struct wrap_object_impl
		{
			static v8::Handle<v8::Value> call(v8::Arguments const& args)
			{
				try
				{
					return singleton::instance().wrap_object(args);
				}
				catch (std::exception const& ex)
				{
					return throw_ex(ex.what());
				}
			}
		};

		// Create function template and call its function to convert arguments array
		// into v8::Arguments and call class_singleton::wrap_object(args)
		v8::Handle<v8::FunctionTemplate> wrapper = v8::FunctionTemplate::New(&wrap_object_impl::call);

		v8::TryCatch try_catch;
		v8::Handle<v8::Value> result = wrapper->GetFunction()->Call(v8::Object::New(), argc, argv);
		if ( try_catch.HasCaught() )
		{
			v8::String::Utf8Value msg(try_catch.Exception());
			throw std::runtime_error(*msg);
		}
		return scope.Close(result->ToObject());
	}
};

} // namespace v8pp

#endif // V8PP_CLASS_HPP_INCLUDED
