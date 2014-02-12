#ifndef V8PP_CLASS_HPP_INCLUDED
#define V8PP_CLASS_HPP_INCLUDED

#include <boost/mpl/front.hpp>
#include <boost/type_traits/is_member_object_pointer.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include "v8pp/factory.hpp"
#include "v8pp/forward.hpp"
#include "v8pp/property.hpp"

namespace v8pp {

template<typename T, typename Factory>
class class_;

namespace detail {

#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
// Global singleton_registry to store class_singleton instances
// in a single place among several DLLs
class singleton_registry
{
public:
	typedef boost::unordered_map<std::type_index, void*> singletons;

	// Get existing instance or create a new one
	template<typename T>
	static T& get()
	{
		std::type_index const type_idx(typeid(T));
		singletons::iterator it = items_.find(type_idx);
		if ( it == items_.end())
		{
			it = items_.insert(std::make_pair(type_idx, new T)).first;
		}
		return *static_cast<T*>(it->second);
	}
private:
	singleton_registry();
	~singleton_registry();
	singleton_registry(singleton_registry const&);
	singleton_registry& operator=(singleton_registry const&);

	static V8PP_API singletons items_;
};
#endif

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
	typedef typename Factory::template instance<T> factory_type;
 
	static T* create(v8::Arguments const& args, boost::false_type)
	{
		return call_from_v8<factory_type>(&factory_type::create, args);
	}

	static T* create(v8::Arguments const& args, boost::true_type)
	{
		return factory_type::create(args);
	}

public:
	class_singleton()
		: func_(v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New()))
	{
		if ( !this->HAS_NULL_FACTORY )
		{
			func_->Inherit(js_function_template());
		}
		func_->InstanceTemplate()->SetInternalFieldCount(1);
	}

	static class_singleton& instance()
	{
#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
		static class_singleton& instance_ = singleton_registry::get<class_singleton>();
#else
		static class_singleton instance_;
#endif
		return instance_;
	}

	v8::Persistent<v8::FunctionTemplate>& class_function_template()
	{
		return func_;
	}

	v8::Persistent<v8::FunctionTemplate>& js_function_template()
	{
		return this->js_function_template_helper();
	}

	v8::Persistent<v8::Object> wrap_object(T* wrap)
	{
		v8::Persistent<v8::Object> obj;
		obj.Reset(v8::Isolate::GetCurrent(), func_->GetFunction()->NewInstance());

		obj->SetAlignedPointerInInternalField(0, wrap);
		detail::object_registry<T>::add(wrap, obj);
		obj.MakeWeak(wrap, on_made_weak);

		return obj;
	}

	v8::Persistent<v8::Object> wrap_external_object(T* wrap)
	{
		v8::Persistent<v8::Object> obj;
		obj.Reset(v8::Isolate::GetCurrent(), func_->GetFunction()->NewInstance());

		obj->SetAlignedPointerInInternalField(0, wrap);
		detail::object_registry<T>::add(wrap, obj);

		return obj;
	}

	v8::Persistent<v8::Object> wrap_object(v8::Arguments const& args)
	{
		T* wrap = create(args, boost::is_same<Factory, v8_args_factory>());
		return wrap_object(wrap);
	}

	void destroy_objects()
	{
		detail::object_registry<T>::remove_all(&factory_type::destroy);
	}

	static void destroy_object(T* obj)
	{
		detail::object_registry<T>::remove(obj, &factory_type::destroy);
	}

private:
	static void on_made_weak(v8::Isolate*, v8::Persistent<v8::Object>* obj, T* native)
	{
		destroy_object(native);
//		no more need to obj->Dispose() since it's performed in destroy_object() call above
	}

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
		static_assert(boost::is_polymorphic<T>::value == boost::is_polymorphic<U>::value,
			"Parent class should be polymorphic too");
		js_function_template()->Inherit(parent.class_function_template());
	}

	// Set class method with any prototype
	template<typename Method>
	typename boost::enable_if<boost::is_member_function_pointer<Method>, class_&>::type
	set(char const *name, Method method)
	{
		v8::HandleScope scope;

		typedef typename detail::mem_function_ptr<Method> MethodProto;

		v8::InvocationCallback callback = forward_mem_function<MethodProto>;
		v8::Handle<v8::Value> data = detail::set_external_data(method);

		class_function_template()->PrototypeTemplate()->Set(
			v8::String::NewSymbol(name),
			v8::FunctionTemplate::New(callback, data));
		return *this;
	}

	// Set static class method with any prototype
	template<typename Function>
	typename boost::enable_if<detail::is_function_pointer<Function>, class_&>::type
	set(char const *name, Function function)
	{
		v8::HandleScope scope;

		typedef typename detail::function_ptr<Function> FunctionProto;

		v8::InvocationCallback callback = forward_function<FunctionProto>;
		v8::Handle<v8::Value> data = detail::set_external_data(function);

		js_function_template()->Set(v8::String::NewSymbol(name),
			v8::FunctionTemplate::New(callback, data));
		return *this;
	}

	// Set class attribute
	template<typename Attribute>
	typename boost::enable_if<boost::is_member_object_pointer<Attribute>, class_&>::type
	set(char const *name, Attribute attribute, bool read_only = false)
	{
		v8::HandleScope scope;

		typedef typename detail::mem_object_ptr<Attribute> AttributeProto;

		v8::AccessorGetter getter = member_get<AttributeProto>;
		v8::AccessorSetter setter = member_set<AttributeProto>;
		if ( read_only )
		{
			setter = NULL;
		}

		v8::Handle<v8::Value> data = detail::set_external_data(attribute);
		v8::PropertyAttribute const prop_attrs = v8::PropertyAttribute(v8::DontDelete | (setter? 0 : v8::ReadOnly));

		class_function_template()->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol(name),
			getter, setter, data, v8::DEFAULT, prop_attrs);
		return *this;
	}

	// Set class attribute with getter and setter
	template<typename GetMethod, typename SetMethod>
	typename boost::enable_if<boost::mpl::and_<
		boost::is_member_function_pointer<GetMethod>,
		boost::is_member_function_pointer<SetMethod> >, class_&>::type
	set(char const *name, property_<GetMethod, SetMethod> prop)
	{
		v8::HandleScope scope;

		v8::AccessorGetter getter = property_<GetMethod, SetMethod>::get;
		v8::AccessorSetter setter = property_<GetMethod, SetMethod>::set;
		if (property_<GetMethod, SetMethod>::is_readonly)
		{
			setter = NULL;
		}

		v8::Handle<v8::Value> data = detail::set_external_data(prop);
		v8::PropertyAttribute const prop_attrs = v8::PropertyAttribute(v8::DontDelete | (setter? 0 : v8::ReadOnly));

		class_function_template()->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol(name),
			getter, setter, data, v8::DEFAULT, prop_attrs);
		return *this;
	}

	// Set value as a read-only property
	template<typename Value>
	class_& set_const(char const* name, Value value)
	{
		v8::HandleScope scope;

		class_function_template()->PrototypeTemplate()->Set(v8::String::NewSymbol(name), v8pp::to_v8(value),
			v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	// create javascript object which references externally created C++
	// class. It will not take ownership of the C++ pointer.
	static v8::Handle<v8::Object> reference_external(T* ext)
	{
		return singleton::instance().wrap_external_object(ext);
	}

	// As ReferenceExternal but delete memory for C++ object when javascript
	// object is deleted. You must use "new" to allocate ext.
	static v8::Handle<v8::Object> import_external(T* ext)
	{
		return singleton::instance().wrap_object(ext);
	}

	static v8::Persistent<v8::FunctionTemplate>& class_function_template()
	{
		return singleton::instance().class_function_template();
	}

	static v8::Persistent<v8::FunctionTemplate>& js_function_template()
	{
		return singleton::instance().js_function_template();
	}

	static void destroy_object(T* obj)
	{
		singleton::instance().destroy_object(obj);
	}

	static void destroy_object(v8::Handle<v8::Object> object)
	{
		singleton::instance().destroy_object(object);
	}

	static void destroy_objects()
	{
		singleton::instance().destroy_objects();
	}

private:
	template<typename P>
	static v8::Handle<v8::Value> member_get(v8::Local<v8::String> name, v8::AccessorInfo const& info)
	{
		typedef typename P::attribute_type attribute_type;

		T const& self = v8pp::from_v8<T const&>(info.This());
		attribute_type ptr = detail::get_external_data<attribute_type>(info.Data());
		return v8pp::to_v8(self.*ptr);
	}

	template<typename P>
	static void member_set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::AccessorInfo const& info)
	{
		typedef typename P::return_type return_type;
		typedef typename P::attribute_type attribute_type;

		T& self = v8pp::from_v8<T&>(info.This());
		attribute_type ptr = detail::get_external_data<attribute_type>(info.Data());
		self.*ptr = v8pp::from_v8<return_type>(value);
	}
};

} // namespace v8pp

#endif // V8PP_CLASS_HPP_INCLUDED
