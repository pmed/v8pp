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
#include "v8pp/persistent_ptr.hpp"

namespace v8pp {

template<typename T>
class class_;

namespace detail {

class class_info : boost::noncopyable
{
public:
	explicit class_info(std::type_info const& type)
		: type_(&type)
	{
	}

	typedef void* (*cast_function)(void* ptr);

	void add_base(class_info* info, std::type_info const& type, cast_function cast)
	{
		base_classes::iterator it = std::find_if(bases_.begin(), bases_.end(),
			[info](base_class_info const& base) { return base.info == info; });
		if (it != bases_.end())
		{
			assert(false && "duplicated inheritance");
			throw std::runtime_error(std::string("duplicated inheritance from ") + it->type->name());
		}
		bases_.emplace_back(base_class_info(info, type, cast));
		info->derivatives_.emplace_back(this);
	}

	bool cast(void*& ptr, std::type_info const& type)
	{
		if (type == *type_)
		{
			return true;
		}

		// fast way - search a direct parent
		for (base_classes::const_iterator it = bases_.begin(), end = bases_.end(); it != end; ++it)
		{
			if (*it->type == type)
			{
				ptr = it->cast(ptr);
				return true;
			}
		}

		// slower way - walk on hierarhy
		for (base_classes::const_iterator it = bases_.begin(), end = bases_.end(); it != end; ++it)
		{
			void* p = it->cast(ptr);
			if (it->info->cast(p, type))
			{
				ptr = p;
				return true;
			}
		}

		return false;
	}

	template<typename T>
	void add_object(v8::Isolate* isolate, T* object, v8::Local<v8::Object> handle)
	{
		assert(objects_.find(object) == objects_.end() && "duplicate object");
		objects_.emplace(object, persistent<v8::Object>(isolate, handle));
	}

	template<typename T>
	void remove_object(v8::Isolate* isolate, T* object, void (*destroy)(v8::Isolate* isolate, T* obj))
	{
		objects::iterator it = objects_.find(object);
		assert(objects_.find(object) != objects_.end() && "no object");
		if (it != objects_.end())
		{
			it->second.Reset();
			if (destroy)
			{
				destroy(isolate, object);
			}
			objects_.erase(it);
		}
	}

	template<typename T>
	void remove_objects(v8::Isolate* isolate, void (*destroy)(v8::Isolate* isolate, T* obj))
	{
		for (objects::iterator it = objects_.begin(), end = objects_.end(); it != end; ++it)
		{
			it->second.Reset();
			if (destroy)
			{
				destroy(isolate, static_cast<T*>(it->first));
			}
		}
		objects_.clear();
	}

	v8::Local<v8::Object> find_object(v8::Isolate* isolate, void const* object)
	{
		objects::iterator it = objects_.find(const_cast<void*>(object));
		if (it != objects_.end())
		{
			return to_local(isolate, it->second);
		}

		v8::Local<v8::Object> result;
		for (derivative_classes::const_iterator it = derivatives_.begin(), end = derivatives_.end();
			it != end && result.IsEmpty(); ++it)
		{
			result = (*it)->find_object(isolate, object);
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

	typedef boost::unordered_map<void*, v8pp::persistent<v8::Object>> objects;
	objects objects_;

	typedef std::vector<base_class_info> base_classes;
	base_classes bases_;

	typedef std::vector<class_info*> derivative_classes;
	derivative_classes derivatives_;
};

template<typename T>
class class_singleton : public class_info
{
private:
	explicit class_singleton(v8::Isolate* isolate)
		: class_info(typeid(T))
		, create_()
		, destroy_()
		, isolate_(isolate)
	{
		v8::Local<v8::FunctionTemplate> func = v8::FunctionTemplate::New(isolate_);
		v8::Local<v8::FunctionTemplate> js_func = v8::FunctionTemplate::New(isolate_, &ctor_function);

		func_.Reset(isolate_, func);
		js_func_.Reset(isolate_, js_func);

		// each JavaScript instance has 2 internal fields:
		//  0 - pointer to a wrapped C++ object
		//  1 - pointer to the class_singleton
		func->InstanceTemplate()->SetInternalFieldCount(2);
	}

public:
	static class_singleton& instance(v8::Isolate* isolate)
	{
		// Get pointer to singletons map from v8::Isolate
		typedef boost::unordered_map<std::type_index, void*> singletons_map;

		singletons_map* singletons = static_cast<singletons_map*>(isolate->GetData(0));
		if (!singletons)
		{
			// No singletons map yet, create and store it
			singletons = new singletons_map;
			isolate->SetData(0, singletons);
		}

		// Get singleton instance from the map
		class_singleton* result;

		std::type_index const class_type = typeid(T);
		singletons_map::iterator it = singletons->find(class_type);
		if (it == singletons->end())
		{
			// No singleton instance, create and add it
			result = new class_singleton(isolate);
			singletons->insert(std::make_pair(class_type, result));
		}
		else
		{
			result = static_cast<class_singleton*>(it->second);
		}
		return *result;
	}

	v8::Isolate* isolate() { return isolate_; }

	v8::Local<v8::FunctionTemplate> class_function_template()
	{
		if (!create_ && !destroy_)
		{
			assert(false && "Specify class .ctor");
			throw std::runtime_error("Specify class .ctor");
		}
		return to_local(isolate_, func_);
	}

	v8::Local<v8::FunctionTemplate> js_function_template()
	{
		return to_local(isolate_, js_func_.IsEmpty()? func_ : js_func_);
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
		class_singleton<U>* base = &class_singleton<U>::instance(isolate_);
		add_base(base, typeid(U), &cast_to<U>);
		js_function_template()->Inherit(base->class_function_template());
	}

	v8::Handle<v8::Object> wrap_external_object(T* wrap)
	{
		v8::EscapableHandleScope scope(isolate_);

		v8::Local<v8::Object> obj = class_function_template()->GetFunction()->NewInstance();
		obj->SetAlignedPointerInInternalField(0, wrap);
		obj->SetAlignedPointerInInternalField(1, this);

		class_info::add_object(isolate_, wrap, obj);

		return scope.Escape(obj);
	}

	v8::Handle<v8::Object> wrap_object(T* wrap)
	{
		v8::EscapableHandleScope scope(isolate_);

		v8::Local<v8::Object> obj = wrap_external_object(wrap);

		v8::Persistent<v8::Object> pobj(isolate_, obj);
		pobj.SetWeak(wrap, on_weak);

		return scope.Escape(obj);
	}

	v8::Handle<v8::Object> wrap_object(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		return create_? wrap_object(create_(args)) : throw std::runtime_error("create is not allowed");
	}

	T* unwrap_object(v8::Local<v8::Value> value)
	{
		v8::HandleScope scope(isolate_);

		while (value->IsObject())
		{
			v8::Handle<v8::Object> obj = value->ToObject();
			if (obj->InternalFieldCount() == 2)
			{
				void* ptr = obj->GetAlignedPointerFromInternalField(0);
				class_info* info = static_cast<class_info*>(obj->GetAlignedPointerFromInternalField(1));
				if (info && info->cast(ptr, typeid(T)))
				{
					return static_cast<T*>(ptr);
				}
			}
			value = obj->GetPrototype();
		}
		return nullptr;
	}

	v8::Handle<v8::Object> find_object(T const* obj)
	{
		return class_info::find_object(isolate_, obj);
	}

	void destroy_objects()
	{
		class_info::remove_objects(isolate_, destroy_);
	}

	void destroy_object(T* obj)
	{
		class_info::remove_object(isolate_, obj, destroy_);
	}

private:
	typedef T* (*create_fun)(v8::FunctionCallbackInfo<v8::Value> const& args);
	typedef void (*destroy_fun)(v8::Isolate* isolate, T* object);

	v8::Isolate* isolate_;

	create_fun create_;
	destroy_fun destroy_;

	template<typename FactoryType>
	static T* create(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		return call_from_v8<FactoryType>(&FactoryType::create, args);
	}

	template<typename Factory>
	void use_factory(Factory)
	{
		typedef typename Factory::template instance<T> factory_type;
		create_ = &create<factory_type>;
		destroy_ = &factory_type::destroy;
		class_function_template()->Inherit(js_function_template());
	}

	void use_factory(v8_args_factory)
	{
		typedef typename v8_args_factory::template instance<T> factory_type;
		create_ = &factory_type::create;
		destroy_ = &factory_type::destroy;
		class_function_template()->Inherit(js_function_template());
	}

	void use_factory(no_ctor_factory)
	{
		typedef typename no_ctor_factory::template instance<T> factory_type;
		create_ = nullptr;
		destroy_ = &factory_type::destroy;
		js_func_.Reset();
	}

	static void ctor_function(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		v8::Isolate* isolate = args.GetIsolate();
		try
		{
			return args.GetReturnValue().Set(instance(isolate).wrap_object(args));
		}
		catch (std::exception const& ex)
		{
			args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	static void on_weak(v8::WeakCallbackData<v8::Object, T> const& data)
	{
		instance(data.GetIsolate()).destroy_object(data.GetParameter());
//		no more need to obj->Dispose() since it's performed in destroy_object() call above
	}

	v8::UniquePersistent<v8::FunctionTemplate> func_;
	v8::UniquePersistent<v8::FunctionTemplate> js_func_;

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
	typedef detail::class_singleton<T> class_singleton;
public:
	// Register with default constructor
	explicit class_(v8::Isolate* isolate)
		: class_singleton_(class_singleton::instance(isolate))
	{
		class_singleton_.template ctor< factory<> >();
	}

	// Register with no constructor
	explicit class_(v8::Isolate* isolate, no_ctor_t)
		: class_singleton_(detail::class_singleton<T>::instance(isolate))
	{
		class_singleton_.template ctor<no_ctor_factory>();
	}

	// Register with v8::Arguments constructor
	explicit class_(v8::Isolate* isolate, v8_args_ctor_t)
		: class_singleton_(detail::class_singleton<T>::instance(isolate))
	{
		class_singleton_.template ctor<v8_args_factory>();
	}

	// Register with arbitrary constructor signature, use ctor<arg1, arg2>()
	template<typename Factory>
	explicit class_(v8::Isolate* isolate, Factory)
		: class_singleton_(detail::class_singleton<T>::instance(isolate))
	{
		class_singleton_.template ctor<Factory>();
	}

	// Inhert class from class U
	template<typename U>
	class_& inherit()
	{
		static_assert(boost::is_base_and_derived<U, T>::value, "Class U should be base for class T");
		//TODO: boost::is_convertible<T*, U*> and check for duplicates in hierarchy?
		class_singleton_.template inherit<U>();
		return *this;
	}

	// Set class method with any prototype
	template<typename Method>
	typename boost::enable_if<boost::is_member_function_pointer<Method>, class_&>::type
	set(char const *name, Method method)
	{
		v8::HandleScope scope(isolate());

		typedef typename detail::mem_function_ptr<Method> MethodProto;

		v8::FunctionCallback callback = forward_mem_function<MethodProto>;
		v8::Handle<v8::Value> data = detail::set_external_data(isolate(), method);

		class_singleton_.class_function_template()->PrototypeTemplate()->Set(
			to_v8(isolate(), name),
			v8::FunctionTemplate::New(isolate(), callback, data));
		return *this;
	}

	// Set static class method with any prototype
	template<typename Function>
	typename boost::enable_if<detail::is_function_pointer<Function>, class_&>::type
	set(char const *name, Function function)
	{
		v8::HandleScope scope(isolate());

		typedef typename detail::function_ptr<Function> FunctionProto;

		v8::FunctionCallback callback = forward_function<FunctionProto>;
		v8::Handle<v8::Value> data = detail::set_external_data(isolate(), function);

		class_singleton_.js_function_template()->Set(to_v8(isolate(), name),
			v8::FunctionTemplate::New(isolate(), callback, data));
		return *this;
	}

	// Set class attribute
	template<typename Attribute>
	typename boost::enable_if<boost::is_member_object_pointer<Attribute>, class_&>::type
	set(char const *name, Attribute attribute, bool read_only = false)
	{
		v8::HandleScope scope(isolate());

		typedef typename detail::mem_object_ptr<Attribute> AttributeProto;

		v8::AccessorGetterCallback getter = member_get<AttributeProto>;
		v8::AccessorSetterCallback setter = member_set<AttributeProto>;
		if ( read_only )
		{
			setter = NULL;
		}

		v8::Handle<v8::Value> data = detail::set_external_data(isolate(), attribute);
		v8::PropertyAttribute const prop_attrs = v8::PropertyAttribute(v8::DontDelete | (setter? 0 : v8::ReadOnly));

		class_singleton_.class_function_template()->PrototypeTemplate()->SetAccessor(to_v8(isolate(), name),
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
		v8::HandleScope scope(isolate());

		v8::AccessorGetterCallback getter = property_<GetMethod, SetMethod>::template get<T>;
		v8::AccessorSetterCallback setter = property_<GetMethod, SetMethod>::template set<T>;
		if (property_<GetMethod, SetMethod>::is_readonly)
		{
			setter = NULL;
		}

		v8::Handle<v8::Value> data = detail::set_external_data(isolate(), prop);
		v8::PropertyAttribute const prop_attrs = v8::PropertyAttribute(v8::DontDelete | (setter? 0 : v8::ReadOnly));

		class_singleton_.class_function_template()->PrototypeTemplate()->SetAccessor(to_v8(isolate(), name),
			getter, setter, data, v8::DEFAULT, prop_attrs);
		return *this;
	}

	// Set value as a read-only property
	template<typename Value>
	class_& set_const(char const* name, Value value)
	{
		v8::HandleScope scope(isolate());

		class_singleton_.class_function_template()->PrototypeTemplate()->Set(to_v8(isolate(), name),
			v8pp::to_v8(isolate(), value), v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	v8::Isolate* isolate() { return class_singleton_.isolate(); }

	v8::Local<v8::FunctionTemplate> class_function_template()
	{
		return class_singleton_.class_function_template();
	}

	v8::Local<v8::FunctionTemplate> js_function_template()
	{
		return class_singleton_.js_function_template();
	}

	// create javascript object which references externally created C++
	// class. It will not take ownership of the C++ pointer.
	static v8::Handle<v8::Object> reference_external(v8::Isolate* isolate, T* ext)
	{
		return class_singleton::instance(isolate).wrap_external_object(ext);
	}

	// As ReferenceExternal but delete memory for C++ object when javascript
	// object is deleted. You must use "new" to allocate ext.
	static v8::Handle<v8::Object> import_external(v8::Isolate* isolate, T* ext)
	{
		return class_singleton::instance(isolate).wrap_object(ext);
	}

	static T* unwrap_object(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		return class_singleton::instance(isolate).unwrap_object(value);
	}

	static v8::Handle<v8::Value> find_object(v8::Isolate* isolate, T const* obj)
	{
		return class_singleton::instance(isolate).find_object(obj);
	}

	static void destroy_object(v8::Isolate* isolate, T* obj)
	{
		class_singleton::instance(isolate).destroy_object(obj);
	}

	static void destroy_objects(v8::Isolate* isolate)
	{
		class_singleton::instance(isolate).destroy_objects();
	}

private:
	template<typename P>
	static void member_get(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		typedef typename P::attribute_type attribute_type;

		v8::Isolate* isolate = info.GetIsolate();

		T const& self = v8pp::from_v8<T const&>(isolate, info.This());
		attribute_type ptr = detail::get_external_data<attribute_type>(info.Data());

		info.GetReturnValue().Set(v8pp::to_v8(isolate, self.*ptr));
	}

	template<typename P>
	static void member_set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		typedef typename P::return_type return_type;
		typedef typename P::attribute_type attribute_type;

		v8::Isolate* isolate = info.GetIsolate();

		T& self = v8pp::from_v8<T&>(isolate, info.This());
		attribute_type ptr = detail::get_external_data<attribute_type>(info.Data());

		self.*ptr = v8pp::from_v8<return_type>(isolate, value);
	}

	detail::class_singleton<T>& class_singleton_;
};

} // namespace v8pp

#endif // V8PP_CLASS_HPP_INCLUDED
