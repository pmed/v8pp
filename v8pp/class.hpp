#ifndef V8PP_CLASS_HPP_INCLUDED
#define V8PP_CLASS_HPP_INCLUDED

#include <typeinfo>

#include <boost/mpl/front.hpp>
#include <boost/type_traits/is_member_object_pointer.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include "v8pp/factory.hpp"
#include "v8pp/forward.hpp"
#include "v8pp/property.hpp"

namespace v8pp {

template<typename T>
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

class class_info
{
public:
	explicit class_info(std::type_info const& type)
		: type_(&type)
	{
	}

	typedef void* (*cast_function)(void* ptr);

	bool has_bases() const { return !bases_.empty(); }

	void add_base(class_info* info, std::type_info const& type, cast_function cast)
	{
		base_classes::iterator it = std::find_if(bases_.begin(), bases_.end(),
			[info](base_class_info const& base) { return base.info == info; });
		if (it != bases_.end())
		{
			assert(false && "duplicated inheritance");
			throw std::runtime_error(std::string("duplicated inheritance from ") + it->type->name());
		}
		bases_.push_back(base_class_info(info, type, cast));
		info->derivatives_.push_back(this);
	}

	bool upcast(void*& ptr, std::type_info const& type)
	{
		if (bases_.empty() || type == *type_)
		{
			return true;
		}

		// fast way - search a direct parent
		for (base_classes::const_iterator it = bases_.begin(), end = bases_.end(); it != end; ++it)
		{
			if (*it->type == *type_)
			{
				ptr = it->cast(ptr);
				return true;
			}
		}

		// slower way - walk on hierarhy
		for (base_classes::const_iterator it = bases_.begin(), end = bases_.end(); it != end; ++it)
		{
			void* p = it->cast(ptr);
			if (it->info->upcast(p, type))
			{
				ptr = p;
				return true;
			}
		}

		return false;
	}

	template<typename T>
	void add_object(T* object, v8::Persistent<v8::Value> handle)
	{
		assert(objects_.find(object) == objects_.end() && "duplicate object");
		objects_.insert(std::make_pair(object, handle));
	}

	template<typename T>
	void remove_object(T* object, void (*destroy)(T* obj))
	{
		objects::iterator it = objects_.find(object);
		assert(objects_.find(object) != objects_.end() && "no object");
		if (it != objects_.end())
		{
			it->second.Dispose();
			objects_.erase(it);
			if (destroy)
			{
				destroy(object);
			}
		}
	}

	template<typename T>
	void remove_objects(void (*destroy)(T* obj))
	{
		for (objects::iterator it = objects_.begin(), end = objects_.end(); it != end; ++it)
		{
			it->second.Dispose();
			if (destroy)
			{
				destroy(static_cast<T*>(it->first));
			}
		}
		objects_.clear();
	}

	v8::Handle<v8::Value> find_object(void const* object)
	{
		objects::const_iterator it = objects_.find(const_cast<void*>(object));
		if (it != objects_.end())
		{
			return it->second;
		}

		v8::Handle<v8::Value> result;
		for (derivative_classes::const_iterator it = derivatives_.begin(), end = derivatives_.end();
			it != end && result.IsEmpty(); ++it)
		{
			result = (*it)->find_object(object);
		}
		return result;
	}

private:
	struct base_class_info
	{
		class_info* info;
		std::type_info const* type;
		cast_function cast;

		base_class_info(class_info* info, std::type_info const& type, cast_function cast)
			: info(info)
			, type(&type)
			, cast(cast)
		{
		}
	};

	std::type_info const* type_;

	typedef boost::unordered_map<void*, v8::Persistent<v8::Value> > objects;
	objects objects_;

	typedef std::vector<base_class_info> base_classes;
	base_classes bases_;

	typedef std::vector<class_info*> derivative_classes;
	derivative_classes derivatives_;
};

template<typename T>
class class_singleton : public class_info
{
public:
	class_singleton()
		: class_info(typeid(T))
		, func_(v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New()))
		, js_func_(v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(&ctor_function)))
		, create_()
		, destroy_()
	{
		// each JavaScrpt instance has 2 internal fields:
		//  0 - pointer to a wrapped C++ object
		//  1 - pointer to the class_singleton
		func_->InstanceTemplate()->SetInternalFieldCount(2);
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
		if (!create_ && !destroy_)
		{
			assert(false && "Specify class .ctor");
			throw std::runtime_error("Specify class .ctor");
		}
		return func_;
	}

	v8::Persistent<v8::FunctionTemplate>& js_function_template()
	{
		return js_func_.IsEmpty()? func_ : js_func_;
	}

	template<typename Factory>
	void ctor()
	{
		if (create_ || destroy_)
		{
			assert(false && ".ctor already set");
			throw std::runtime_error(".ctor already set");
		}
		use_factory(Factory());
	}

	template<typename U>
	void inherit()
	{
		class_singleton<U>* base = &class_singleton<U>::instance();
		add_base(base, typeid(U), &cast_to<U>);
		js_function_template()->Inherit(base->class_function_template());
	}

	v8::Persistent<v8::Object> wrap_external_object(T* wrap)
	{
		v8::HandleScope scope;

		v8::Persistent<v8::Object> obj;
		obj.Reset(v8::Isolate::GetCurrent(), func_->GetFunction()->NewInstance());

		obj->SetAlignedPointerInInternalField(0, wrap);
		obj->SetAlignedPointerInInternalField(1, this);
		class_info::add_object(wrap, obj);

		return obj;
	}

	v8::Persistent<v8::Object> wrap_object(T* wrap)
	{
		v8::Persistent<v8::Object> obj = wrap_external_object(wrap);
		obj.MakeWeak(wrap, on_made_weak);

		return obj;
	}

	v8::Persistent<v8::Object> wrap_object(v8::Arguments const& args)
	{
		return create_? wrap_object(create_(args)) : throw std::runtime_error("create is not allowed");
	}

	T* unwrap_object(v8::Handle<v8::Value> value)
	{
		v8::HandleScope scope;
		while (value->IsObject())
		{
			v8::Handle<v8::Object> obj = value->ToObject();
			if (obj->InternalFieldCount() == 2)
			{
				void* ptr = obj->GetAlignedPointerFromInternalField(0);
				class_info* info = static_cast<class_info*>(obj->GetAlignedPointerFromInternalField(1));
				if (info && info->has_bases())
				{
					info->upcast(ptr, typeid(T));
				}
				return static_cast<T*>(ptr);
			}
			value = obj->GetPrototype();
		}
		return nullptr;
	}

	v8::Handle<v8::Value> find_object(T const* obj)
	{
		return class_info::find_object(obj);
	}

	void destroy_objects()
	{
		class_info::remove_objects(destroy_);
	}

	void destroy_object(T* obj)
	{
		class_info::remove_object(obj, destroy_);
	}

private:
	typedef T* (*create_fun)(v8::Arguments const& args);
	typedef void (*destroy_fun)(T* object);

	create_fun create_;
	destroy_fun destroy_;

	template<typename FactoryType>
	static T* create(v8::Arguments const& args)
	{
		return call_from_v8<FactoryType>(&FactoryType::create, args);
	}

	template<typename Factory>
	void use_factory(Factory)
	{
		typedef typename Factory::template instance<T> factory_type;
		create_ = &create<factory_type>;
		destroy_ = &factory_type::destroy;
		func_->Inherit(js_function_template());
	}

	void use_factory(v8_args_factory)
	{
		typedef typename v8_args_factory::template instance<T> factory_type;
		create_ = &factory_type::create;
		destroy_ = &factory_type::destroy;
		func_->Inherit(js_function_template());
	}

	void use_factory(no_factory)
	{
		typedef typename no_factory::template instance<T> factory_type;
		create_ = nullptr;
		destroy_ = &factory_type::destroy;
		js_func_.Dispose(); js_func_.Clear();
	}

	static v8::Handle<v8::Value> ctor_function(v8::Arguments const& args)
	{
		try
		{
			return instance().wrap_object(args);
		}
		catch (std::exception const& ex)
		{
			return throw_ex(ex.what());
		}
	}

	static void on_made_weak(v8::Isolate*, v8::Persistent<v8::Object>* obj, T* native)
	{
		instance().destroy_object(native);
//		no more need to obj->Dispose() since it's performed in destroy_object() call above
	}

	v8::Persistent<v8::FunctionTemplate> func_;
	v8::Persistent<v8::FunctionTemplate> js_func_;

	template<typename U>
	static void* cast_to(void* ptr)
	{
		return static_cast<U*>(static_cast<T*>(ptr));
	}
};

} // namespace detail

// Interface for registering C++ classes in V8
template<typename T>
class class_
{
	typedef detail::class_singleton<T> singleton;
public:
	// Register with default constructor
	class_()
	{
		singleton::instance().template ctor< factory<> >();
	}

	// Register with no constructor
	explicit class_(no_ctor_t)
	{
		singleton::instance().template ctor<no_factory>();
	}

	// Register with v8::Arguments constructor
	explicit class_(v8_args_ctor_t)
	{
		singleton::instance().template ctor<v8_args_factory>();
	}

	// Register with arbitrary constructor signature, use ctor<arg1, arg2>()
	template<typename Factory>
	explicit class_(Factory)
	{
		singleton::instance().template ctor<Factory>();
	}

	// Inhert class from class U
	template<typename U>
	class_& inherit()
	{
		static_assert(boost::is_base_and_derived<U, T>::value, "Class U should be base for class T");
		//TODO: boost::is_convertible<T*, U*> and check for duplicates in hierarchy?
		singleton::instance().template inherit<U>();
		return *this;
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

	static T* unwrap_object(v8::Handle<v8::Value> value)
	{
		return singleton::instance().unwrap_object(value);
	}

	static v8::Handle<v8::Value> find_object(T const* obj)
	{
		return singleton::instance().find_object(obj);
	}

	static void destroy_object(T* obj)
	{
		singleton::instance().destroy_object(obj);
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
