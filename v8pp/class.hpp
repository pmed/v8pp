//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_CLASS_HPP_INCLUDED
#define V8PP_CLASS_HPP_INCLUDED

#include <cstdio>

#include <algorithm>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "v8pp/config.hpp"
#include "v8pp/factory.hpp"
#include "v8pp/function.hpp"
#include "v8pp/persistent.hpp"
#include "v8pp/property.hpp"

namespace v8pp {

namespace detail {

inline void* pointer_id(void* ptr) { return ptr; }
inline void* pointer_id(std::shared_ptr<void> const& ptr) { return ptr.get(); }

inline std::string pointer_str(void const* ptr)
{
	std::string buf(sizeof(void*) * 2 + 3, 0); // +3 for 0x and \0 terminator
	int const len =
#if defined(_MSC_VER) && (_MSC_VER < 1900)
		sprintf_s(&buf[0], buf.size(), "%p", ptr);
#else
		snprintf(&buf[0], buf.size(), "%p", ptr);
#endif
	buf.resize(len < 0 ? 0 : len);
	return buf;
}

inline void* const_pointer_cast(void const* ptr)
{
	return const_cast<void*>(ptr);
}

inline std::shared_ptr<void> const_pointer_cast(std::shared_ptr<void const> const& ptr)
{
	return std::const_pointer_cast<void>(ptr);
}

template<typename T>
T* static_pointer_cast(void* ptr)
{
	return static_cast<T*>(ptr);
}

template<typename T>
T const* static_pointer_cast(void const* ptr)
{
	return static_cast<T const*>(ptr);
}

template<typename T>
std::shared_ptr<T> static_pointer_cast(std::shared_ptr<void> const& ptr)
{
	return std::static_pointer_cast<T>(ptr);
}

template<typename T>
std::shared_ptr<T const> static_pointer_cast(std::shared_ptr<void const> const& ptr)
{
	return std::static_pointer_cast<T const>(ptr);
}

struct class_info
{
	type_info const type;
	bool const is_shared_ptr;

	class_info(type_info const& type, bool is_shared_ptr)
		: type(type)
		, is_shared_ptr(is_shared_ptr)
	{
	}

	virtual ~class_info() = default;

	std::string class_name() const
	{
		return "v8pp::class_<" + type.name()
			+ (is_shared_ptr ? ", use_shared_ptr>" : ">");
	}
};

template<bool use_shared_ptr>
class object_registry : public class_info
{
public:
	using pointer_type = typename std::conditional<use_shared_ptr,
		std::shared_ptr<void>, void*>::type;
	using const_pointer_type = typename std::conditional<use_shared_ptr,
		std::shared_ptr<void const>, void const*>::type;

	using cast_function = pointer_type(*)(pointer_type const&);
	using destroy_function = void(*)(v8::Isolate*, pointer_type const&);
	using object_id = void*; //TODO: make opaque type

	explicit object_registry(type_info type)
		: class_info(type, use_shared_ptr)
	{
	}

	void add_base(object_registry* info, cast_function cast)
	{
		auto it = std::find_if(bases_.begin(), bases_.end(),
			[info](base_class_info const& base)
		{
			return base.info == info;
		});
		if (it != bases_.end())
		{
			//assert(false && "duplicated inheritance");
			throw std::runtime_error(this->class_name()
				+ " is already inherited from " + info->class_name());
		}
		bases_.emplace_back(info, cast);
		info->derivatives_.emplace_back(this);
	}

	void add_object(pointer_type const& object, persistent<v8::Object>&& handle)
	{
		auto it = objects_.find(object);
		if (it != objects_.end())
		{
			//assert(false && "duplicate object");
			throw std::runtime_error(this->class_name()
				+ " duplicate object " + pointer_str(pointer_id(object)));
		}
		objects_.emplace(object, std::move(handle));
	}

	void remove_object(v8::Isolate* isolate, object_id id, destroy_function destroy)
	{
		auto it = objects_.find(key(id, std::integral_constant<bool, use_shared_ptr>{}));
		assert(it != objects_.end() && "no object");
		if (it != objects_.end())
		{
			if (!it->second.IsNearDeath())
			{
				// remove pointer to wrapped  C++ object from V8 Object
				// internal field to disable unwrapping for this V8 Object
				assert(to_local(isolate, it->second)
					->GetAlignedPointerFromInternalField(0) == id);
				to_local(isolate, it->second)
					->SetAlignedPointerInInternalField(0, nullptr);
			}
			it->second.Reset();
			if (destroy)
			{
				destroy(isolate, it->first);
			}
			objects_.erase(it);
		}
	}

	void remove_objects(v8::Isolate* isolate, destroy_function destroy)
	{
		for (auto& object : objects_)
		{
			object.second.Reset();
			if (destroy)
			{
				destroy(isolate, object.first);
			}
		}
		objects_.clear();
	}

	pointer_type find_object(object_id id) const
	{
		auto it = objects_.find(key(id, std::integral_constant<bool, use_shared_ptr>{}));
		if (it != objects_.end())
		{
			pointer_type ptr = it->first;
			if (cast(ptr, type))
			{
				return ptr;
			}
		}
		return nullptr;
	}

	v8::Local<v8::Object> find_object(v8::Isolate* isolate, object_id id) const
	{
		auto it = objects_.find(key(id, std::integral_constant<bool, use_shared_ptr>{}));
		if (it != objects_.end())
		{
			return to_local(isolate, it->second);
		}

		v8::Local<v8::Object> result;
		for (auto const info : derivatives_)
		{
			result = info->find_object(isolate, id);
			if (!result.IsEmpty()) break;
		}
		return result;
	}

private:
	bool cast(pointer_type& ptr, type_info const& type) const
	{
		if (this->type == type || !ptr)
		{
			return true;
		}

		// fast way - search a direct parent
		for (base_class_info const& base : bases_)
		{
			if (base.info->type == type)
			{
				ptr = base.cast(ptr);
				return true;
			}
		}

		// slower way - walk on hierarhy
		for (base_class_info const& base : bases_)
		{
			pointer_type p = base.cast(ptr);
			if (base.info->cast(p, type))
			{
				ptr = p;
				return true;
			}
		}
		return false;
	}

	struct base_class_info
	{
		object_registry* info;
		cast_function cast;

		base_class_info(object_registry* info, cast_function cast)
			: info(info)
			, cast(cast)
		{
		}
	};

	std::vector<base_class_info> bases_;
	std::vector<object_registry*> derivatives_;

	static std::shared_ptr<void> key(object_id id, std::true_type)
	{
		return std::shared_ptr<void>(id, [](void*){});
	}

	static void* key(object_id id, std::false_type)
	{
		return id;
	}

	std::unordered_map<pointer_type, persistent<v8::Object>> objects_;
};

template<typename T, bool use_shared_ptr>
class class_singleton;

class class_singletons
{
public:
	template<typename T, bool use_shared_ptr>
	static class_singleton<T, use_shared_ptr>& add_class(v8::Isolate* isolate)
	{
		class_singletons* singletons = instance(add, isolate);
		type_info const type = type_id<T>();
		auto it = singletons->find(type);
		if (it != singletons->classes_.end())
		{
			//assert(false && "class already registred");
			throw std::runtime_error(class_info(type, use_shared_ptr).class_name()
				+ " is already exist in isolate " + pointer_str(isolate));
		}

		using singleton_type = class_singleton<T, use_shared_ptr>;
		singletons->classes_.emplace_back(new singleton_type(isolate, type));
		return *static_cast<singleton_type*>(singletons->classes_.back().get());
	}

	template<typename T, bool use_shared_ptr>
	static void remove_class(v8::Isolate* isolate)
	{
		class_singletons* singletons = instance(get, isolate);
		if (singletons)
		{
			type_info const type = type_id<T>();
			auto it = singletons->find(type);
			if (it != singletons->classes_.end())
			{
				if ((*it)->is_shared_ptr != use_shared_ptr)
				{
					throw std::runtime_error((*it)->class_name()
						+ " is already registered in isolate "
						+ pointer_str(isolate) + " before of "
						+ class_info(type, use_shared_ptr).class_name());
				}
				singletons->classes_.erase(it);
				if (singletons->classes_.empty())
				{
					instance(remove, isolate);
				}
			}
		}
	}

	template<typename T, bool use_shared_ptr>
	static class_singleton<T, use_shared_ptr>& find_class(v8::Isolate* isolate)
	{
		class_singletons* singletons = instance(get, isolate);
		type_info const type = type_id<T>();
		if (singletons)
		{
			auto it = singletons->find(type);
			if (it != singletons->classes_.end())
			{
				if ((*it)->is_shared_ptr != use_shared_ptr)
				{
					throw std::runtime_error((*it)->class_name()
						+ " is already registered in isolate "
						+ pointer_str(isolate) + " before of "
						+ class_info(type, use_shared_ptr).class_name());
				}
				return *static_cast<class_singleton<T, use_shared_ptr>*>(it->get());
			}
		}
		//assert(false && "class not registered");
		throw std::runtime_error(class_info(type, use_shared_ptr).class_name()
			+ " is not registered in isolate " + pointer_str(isolate));
	}

	static void remove_all(v8::Isolate* isolate)
	{
		instance(remove, isolate);
	}

private:
	using classes = std::vector<std::unique_ptr<class_info>>;
	classes classes_;

	classes::iterator find(type_info const& type)
	{
		return std::find_if(classes_.begin(), classes_.end(),
			[&type](std::unique_ptr<class_info> const& info)
			{
				return info->type == type;
			});
	}

	enum operation { get, add, remove };
	static class_singletons* instance(operation op, v8::Isolate* isolate)
	{
#if defined(V8PP_ISOLATE_DATA_SLOT)
		class_singletons* instances = static_cast<class_singletons*>(
			isolate->GetData(V8PP_ISOLATE_DATA_SLOT));
		switch (op)
		{
		case get:
			return instances;
		case add:
			if (!instances)
			{
				instances = new class_singletons;
				isolate->SetData(V8PP_ISOLATE_DATA_SLOT, instances);
			}
			return instances;
		case remove:
			if (instances)
			{
				delete instances;
				isolate->SetData(V8PP_ISOLATE_DATA_SLOT, nullptr);
			}
		default:
			return nullptr;
		}
#else
		static std::unordered_map<v8::Isolate*, class_singletons> instances;
		switch (op)
		{
		case get:
			{
				auto it = instances.find(isolate);
				return it != instances.end()? &it->second : nullptr;
			}
		case add:
			return &instances[isolate];
		case remove:
			instances.erase(isolate);
		default:
			return nullptr;
		}
#endif
	}
};

template<typename T, bool use_shared_ptr>
class class_singleton : public object_registry<use_shared_ptr>
{
public:
	using typename object_registry<use_shared_ptr>::object_id;
	using typename object_registry<use_shared_ptr>::pointer_type;
	using typename object_registry<use_shared_ptr>::const_pointer_type;

	using object_pointer_type = decltype(
		static_pointer_cast<T>(std::declval<pointer_type>()));
	using object_const_pointer_type = decltype(
		static_pointer_cast<T>(std::declval<const_pointer_type>()));

	class_singleton(v8::Isolate* isolate, type_info const& type)
		: object_registry<use_shared_ptr>(type)
		, isolate_(isolate)
	{
		v8::Local<v8::FunctionTemplate> func = v8::FunctionTemplate::New(isolate_);
		v8::Local<v8::FunctionTemplate> js_func = v8::FunctionTemplate::New(isolate_,
			[](v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			v8::Isolate* isolate = args.GetIsolate();
			try
			{
				return args.GetReturnValue().Set(
					class_singletons::find_class<T, use_shared_ptr>(isolate)
						.wrap_object(args));
			}
			catch (std::exception const& ex)
			{
				args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		});

		func_.Reset(isolate_, func);
		js_func_.Reset(isolate_, js_func);

		// each JavaScript instance has 2 internal fields:
		//  0 - id of a wrapped C++ object
		//  1 - pointer to the class_singleton
		func->InstanceTemplate()->SetInternalFieldCount(2);

		// no class constructor available by default
		ctor_ = nullptr;
		// use destructor from factory<T>::destroy()
		dtor_ = [](v8::Isolate* isolate, pointer_type const& obj)
		{
			factory<T, use_shared_ptr>::destroy(isolate, static_pointer_cast<T>(obj));
		};
	}

	class_singleton(class_singleton const&) = delete;
	class_singleton& operator=(class_singleton const&) = delete;

	~class_singleton()
	{
		destroy_objects();
	}

	v8::Handle<v8::Object> wrap(object_pointer_type const& object, bool destroy_after)
	{
		v8::EscapableHandleScope scope(isolate_);

		v8::Local<v8::Object> obj = class_function_template()
			->GetFunction()->NewInstance();

		persistent<v8::Object> pobj(isolate_, obj);

		object_id id = pointer_id(object);
		obj->SetAlignedPointerInInternalField(0, id);
		obj->SetAlignedPointerInInternalField(1, this);

		if (destroy_after)
		{
			pobj.SetWeak(id,
#ifdef V8_USE_WEAK_CB_INFO
				[](v8::WeakCallbackInfo<void> const& data)
#else
				[](v8::WeakCallbackData<v8::Object, void> const& data)
#endif
			{
				v8::Isolate* isolate = data.GetIsolate();
				object_id object = data.GetParameter();
				class_singletons::find_class<T, use_shared_ptr>(isolate)
					.destroy_object(object);
			}
#ifdef V8_USE_WEAK_CB_INFO
				, v8::WeakCallbackType::kParameter
#endif
				);
		}
		else
		{
			pobj.SetWeak(id,
#ifdef V8_USE_WEAK_CB_INFO
				[](v8::WeakCallbackInfo<void> const& data)
#else
				[](v8::WeakCallbackData<v8::Object, void> const& data)
#endif
			{
				v8::Isolate* isolate = data.GetIsolate();
				object_id object = data.GetParameter();
				class_singletons::find_class<T, use_shared_ptr>(isolate)
					.remove_object(object);
			}
#ifdef V8_USE_WEAK_CB_INFO
				, v8::WeakCallbackType::kParameter
#endif
				);
		}

		object_registry<use_shared_ptr>::add_object(object, std::move(pobj));

		return scope.Escape(obj);
	}

	v8::Isolate* isolate() { return isolate_; }

	v8::Local<v8::FunctionTemplate> class_function_template()
	{
		return to_local(isolate_, func_);
	}

	v8::Local<v8::FunctionTemplate> js_function_template()
	{
		return to_local(isolate_, js_func_.IsEmpty()? func_ : js_func_);
	}

	template<typename ...Args>
	void ctor()
	{
		ctor_ = [](v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			using ctor_type = object_pointer_type (*)(v8::Isolate* isolate, Args...);
			return call_from_v8(static_cast<ctor_type>(
				&factory<T, use_shared_ptr>::create), args);
		};
		class_function_template()->Inherit(js_function_template());
	}

	template<typename U>
	void inherit()
	{
		class_singleton<U, use_shared_ptr>* base =
			&class_singletons::find_class<U, use_shared_ptr>(isolate_);
		this->add_base(base, [](pointer_type const& ptr) -> pointer_type
			{
				return pointer_type(static_pointer_cast<U>(
					static_pointer_cast<T>(ptr)));
			});
		js_function_template()->Inherit(base->class_function_template());
	}

	v8::Handle<v8::Object> wrap_external_object(object_pointer_type const& object)
	{
		return wrap(object, false);
	}

	v8::Handle<v8::Object> wrap_object(object_pointer_type const& object)
	{
		return wrap(object, true);
	}

	v8::Handle<v8::Object> wrap_object(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		if (!ctor_)
		{
			//assert(false && "create not allowed");
			throw std::runtime_error(this->class_name() + " has no constructor");
		}
		return wrap_object(ctor_(args));
	}

	object_pointer_type unwrap_object(v8::Local<v8::Value> value)
	{
		v8::HandleScope scope(isolate_);

		while (value->IsObject())
		{
			v8::Handle<v8::Object> obj = value->ToObject();
			if (obj->InternalFieldCount() == 2)
			{
				object_id id = obj->GetAlignedPointerFromInternalField(0);
				auto registry = static_cast<object_registry<use_shared_ptr>*>(
					obj->GetAlignedPointerFromInternalField(1));
				if (registry)
				{
					pointer_type ptr = registry->find_object(id);
					if (ptr)
					{
						return static_pointer_cast<T>(ptr);
					}
				}
			}
			value = obj->GetPrototype();
		}
		return nullptr;
	}

	v8::Handle<v8::Object> find_object(object_id object)
	{
		return object_registry<use_shared_ptr>::find_object(isolate_, object);
	}

	void remove_object(object_id object)
	{
		object_registry<use_shared_ptr>::remove_object(isolate_, object, nullptr);
	}

	void destroy_objects()
	{
		object_registry<use_shared_ptr>::remove_objects(isolate_, dtor_);
	}

	void destroy_object(object_id object)
	{
		object_registry<use_shared_ptr>::remove_object(isolate_, object, dtor_);
	}

private:
	v8::Isolate* isolate_;
	object_pointer_type (*ctor_)(v8::FunctionCallbackInfo<v8::Value> const& args);
	typename object_registry<use_shared_ptr>::destroy_function dtor_;

	v8::UniquePersistent<v8::FunctionTemplate> func_;
	v8::UniquePersistent<v8::FunctionTemplate> js_func_;
};

} // namespace detail

/// Interface to access C++ classes bound to V8
template<typename T, bool use_shared_ptr = false>
class class_
{
	static_assert(is_wrapped_class<T>::value, "T must be a user-defined class");
	using class_singleton = detail::class_singleton<T, use_shared_ptr>;
	class_singleton& class_singleton_;
public:
	using object_pointer_type = typename class_singleton::object_pointer_type;
	using object_const_pointer_type = typename class_singleton::object_const_pointer_type;

	explicit class_(v8::Isolate* isolate)
		: class_singleton_(detail::class_singletons::add_class<T, use_shared_ptr>(isolate))
	{
	}

	/// Set class constructor signature
	template<typename ...Args>
	class_& ctor()
	{
		class_singleton_.template ctor<Args...>();
		return *this;
	}

	/// Inhert from C++ class U
	template<typename U>
	class_& inherit()
	{
		static_assert(std::is_base_of<U, T>::value,
			"Class U should be base for class T");
		//TODO: std::is_convertible<T*, U*> and check for duplicates in hierarchy?
		class_singleton_.template inherit<U>();
		return *this;
	}

	/// Set C++ class member function
	template<typename Method>
	typename std::enable_if<
		std::is_member_function_pointer<Method>::value, class_&>::type
	set(char const *name, Method mem_func)
	{
		using mem_func_type =
			typename detail::function_traits<Method>::template pointer_type<T>;

		class_singleton_.class_function_template()->PrototypeTemplate()->Set(
			isolate(), name, v8::FunctionTemplate::New(isolate(),
				&detail::forward_function<mem_func_type, use_shared_ptr>,
				detail::set_external_data(isolate(), std::forward<Method>(mem_func))));
		return *this;
	}

	/// Set static class function
	template<typename Function,
		typename Func = typename std::decay<Function>::type>
	typename std::enable_if<detail::is_callable<Func>::value, class_&>::type
	set(char const *name, Function&& func)
	{
		class_singleton_.js_function_template()->Set(isolate(), name,
			wrap_function_template(isolate(), std::forward<Func>(func)));
		return *this;
	}

	/// Set class member data
	template<typename Attribute>
	typename std::enable_if<
		std::is_member_object_pointer<Attribute>::value, class_&>::type
	set(char const *name, Attribute attribute, bool readonly = false)
	{
		v8::HandleScope scope(isolate());

		using attribute_type = typename
			detail::function_traits<Attribute>::template pointer_type<T>;
		v8::AccessorGetterCallback getter = &member_get<attribute_type>;
		v8::AccessorSetterCallback setter = &member_set<attribute_type>;
		if (readonly)
		{
			setter = nullptr;
		}

		class_singleton_.class_function_template()->PrototypeTemplate()
			->SetAccessor(v8pp::to_v8(isolate(), name), getter, setter,
				detail::set_external_data(isolate(),
					std::forward<Attribute>(attribute)), v8::DEFAULT,
				v8::PropertyAttribute(v8::DontDelete | (setter? 0 : v8::ReadOnly)));
		return *this;
	}

	/// Set read/write class property with getter and setter
	template<typename GetMethod, typename SetMethod>
	typename std::enable_if<std::is_member_function_pointer<GetMethod>::value
		&& std::is_member_function_pointer<SetMethod>::value, class_&>::type
	set(char const *name, property_<GetMethod, SetMethod>&& property)
	{
		using prop_type = property_<GetMethod, SetMethod>;

		v8::HandleScope scope(isolate());

		using property_type = property_<
			typename detail::function_traits<GetMethod>::template pointer_type<T>,
			typename detail::function_traits<SetMethod>::template pointer_type<T>
		>;
		v8::AccessorGetterCallback getter = property_type::template get<use_shared_ptr>;
		v8::AccessorSetterCallback setter = property_type::template set<use_shared_ptr>;
		if (prop_type::is_readonly)
		{
			setter = nullptr;
		}

		class_singleton_.class_function_template()->PrototypeTemplate()
			->SetAccessor(v8pp::to_v8(isolate(), name),getter, setter,
				detail::set_external_data(isolate(),
					std::forward<prop_type>(property)), v8::DEFAULT,
				v8::PropertyAttribute(v8::DontDelete | (setter ? 0 : v8::ReadOnly)));
		return *this;
	}

	/// Set value as a read-only property
	template<typename Value>
	class_& set_const(char const* name, Value const& value)
	{
		v8::HandleScope scope(isolate());

		class_singleton_.class_function_template()->PrototypeTemplate()
			->Set(v8pp::to_v8(isolate(), name), to_v8(isolate(), value),
				v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	/// v8::Isolate where the class bindings belongs
	v8::Isolate* isolate() { return class_singleton_.isolate(); }

	v8::Local<v8::FunctionTemplate> class_function_template()
	{
		return class_singleton_.class_function_template();
	}

	v8::Local<v8::FunctionTemplate> js_function_template()
	{
		return class_singleton_.js_function_template();
	}

	/// Create JavaScript object which references externally created C++ class.
	/// It will not take ownership of the C++ pointer.
	static v8::Handle<v8::Object> reference_external(v8::Isolate* isolate,
		object_pointer_type const& ext)
	{
		return detail::class_singletons::find_class<T, use_shared_ptr>(isolate)
			.wrap_external_object(ext);
	}

	/// Remove external reference from JavaScript
	static void unreference_external(v8::Isolate* isolate, object_pointer_type const& ext)
	{
		return detail::class_singletons::find_class<T, use_shared_ptr>(isolate)
			.remove_object(detail::pointer_id(ext));
	}

	/// As reference_external but delete memory for C++ object
	/// when JavaScript object is deleted. You must use "new" to allocate ext.
	static v8::Handle<v8::Object> import_external(v8::Isolate* isolate,
		object_pointer_type const& ext)
	{
		return detail::class_singletons::find_class<T, use_shared_ptr>(isolate)
			.wrap_object(ext);
	}

	/// Get wrapped object from V8 value, may return nullptr on fail.
	static object_pointer_type unwrap_object(v8::Isolate* isolate,
		v8::Handle<v8::Value> value)
	{
		return detail::class_singletons::find_class<T, use_shared_ptr>(isolate)
			.unwrap_object(value);
	}

	/// Create a wrapped C++ object and import it into JavaScript
	template<typename ...Args>
	static v8::Handle<v8::Object> create_object(v8::Isolate* isolate, Args... args)
	{
		return import_external(isolate,
			factory<T, use_shared_ptr>::create(isolate, std::forward<Args>(args)...));
	}

	/// Find V8 object handle for a wrapped C++ object, may return empty handle on fail.
	static v8::Handle<v8::Object> find_object(v8::Isolate* isolate,
		object_const_pointer_type const& obj)
	{
		return detail::class_singletons::find_class<T, use_shared_ptr>(isolate)
			.find_object(detail::pointer_id(detail::const_pointer_cast(obj)));
	}

	/// Destroy wrapped C++ object
	static void destroy_object(v8::Isolate* isolate, object_pointer_type const& obj)
	{
		detail::class_singletons::find_class<T, use_shared_ptr>(isolate)
			.destroy_object(detail::pointer_id(obj));
	}

	/// Destroy all wrapped C++ objects of this class
	static void destroy_objects(v8::Isolate* isolate)
	{
		detail::class_singletons::find_class<T, use_shared_ptr>(isolate)
			.destroy_objects();
	}

	/// Destroy all wrapped C++ objects and this binding class_
	static void destroy(v8::Isolate* isolate)
	{
		detail::class_singletons::remove_class<T, use_shared_ptr>(isolate);
	}

private:
	template<typename Attribute>
	static void member_get(v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		auto self = v8pp::class_<T, use_shared_ptr>::unwrap_object(isolate, info.This());
		Attribute attr = detail::get_external_data<Attribute>(info.Data());
		info.GetReturnValue().Set(to_v8(isolate, (*self).*attr));
	}

	template<typename Attribute>
	static void member_set(v8::Local<v8::String>, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		auto self = v8pp::class_<T, use_shared_ptr>::unwrap_object(isolate, info.This());
		Attribute ptr = detail::get_external_data<Attribute>(info.Data());
		using attr_type = typename detail::function_traits<Attribute>::return_type;
		(*self).*ptr = v8pp::from_v8<attr_type>(isolate, value);
	}
};

/// Interface to access C++ classes bound to V8
/// Objects are stored in std::shared_ptr
template<typename T>
using shared_class = class_<T, true>;

inline void cleanup(v8::Isolate* isolate)
{
	detail::class_singletons::remove_all(isolate);
}

} // namespace v8pp

#endif // V8PP_CLASS_HPP_INCLUDED
