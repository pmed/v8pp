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

template<typename T>
class class_;

namespace detail {

inline std::string class_name(type_info const& info)
{
	return "v8pp::class_<" + info.name() + '>';
}

inline std::string pointer_str(void const* ptr)
{
	std::string buf(sizeof(void*) * 2 + 3, 0); // +3 for 0x and \0 terminator
	int const len = 
#if defined(_MSC_VER) && (_MSC_VER < 1900)
		sprintf_s(&buf[0], buf.size(), "%p", ptr);
#else
		snprintf(&buf[0], buf.size(), "%p", ptr);
#endif
	buf.resize(len < 0? 0 : len);
	return buf;
}

class class_info
{
public:
	explicit class_info(type_info const& type) : type_(type) {}
	class_info(class_info const&) = delete;
	class_info& operator=(class_info const&) = delete;
	virtual ~class_info() = default;

	type_info const& type() const { return type_; }

	using cast_function = void* (*)(void* ptr);

	void add_base(class_info* info, cast_function cast)
	{
		auto it = std::find_if(bases_.begin(), bases_.end(),
			[info](base_class_info const& base) { return base.info == info; });
		if (it != bases_.end())
		{
			//assert(false && "duplicated inheritance");
			throw std::runtime_error(class_name(type_)
				+ " is already inherited from " + class_name(info->type_));
		}
		bases_.emplace_back(info, cast);
		info->derivatives_.emplace_back(this);
	}

	bool cast(void*& ptr, type_info const& type) const
	{
		if (type == type_ || !ptr)
		{
			return true;
		}

		// fast way - search a direct parent
		for (base_class_info const& base : bases_)
		{
			if (base.info->type_ == type)
			{
				ptr = base.cast(ptr);
				return true;
			}
		}

		// slower way - walk on hierarhy
		for (base_class_info const& base : bases_)
		{
			void* p = base.cast(ptr);
			if (base.info->cast(p, type))
			{
				ptr = p;
				return true;
			}
		}

		return false;
	}

	void add_object(void* object, persistent<v8::Object>&& handle)
	{
		auto it = objects_.find(object);
		if (it != objects_.end())
		{
			//assert(false && "duplicate object");
			throw std::runtime_error(class_name(type())
				+ " duplicate object " + pointer_str(object));
		}
		objects_.emplace(object, std::move(handle));
	}

	void remove_object(v8::Isolate* isolate, void* object,
		void (*destroy)(v8::Isolate* isolate, void* obj) = nullptr)
	{
		auto it = objects_.find(object);
		assert(objects_.find(object) != objects_.end() && "no object");
		if (it != objects_.end())
		{
			if (!it->second.IsNearDeath())
			{
				// remove pointer to wrapped  C++ object from V8 Object
				// internal field to disable unwrapping for this V8 Object
				assert(to_local(isolate, it->second)
					->GetAlignedPointerFromInternalField(0) == object);
				to_local(isolate, it->second)
					->SetAlignedPointerInInternalField(0, nullptr);
			}
			if (destroy && !it->second.IsIndependent())
			{
				destroy(isolate, object);
			}
			it->second.Reset();
			objects_.erase(it);
		}
	}

	void remove_objects(v8::Isolate* isolate,
		void (*destroy)(v8::Isolate* isolate, void* obj))
	{
		for (auto& object : objects_)
		{
			if (destroy && !object.second.IsIndependent())
			{
				destroy(isolate, object.first);
			}
			object.second.Reset();
		}
		objects_.clear();
	}

	v8::Local<v8::Object> find_object(v8::Isolate* isolate, void const* object) const
	{
		auto it = objects_.find(const_cast<void*>(object));
		if (it != objects_.end())
		{
			return to_local(isolate, it->second);
		}

		v8::Local<v8::Object> result;
		for (class_info const* info : derivatives_)
		{
			result = info->find_object(isolate, object);
			if (!result.IsEmpty()) break;
		}
		return result;
	}

private:
	struct base_class_info
	{
		class_info* info;
		cast_function cast;

		base_class_info(class_info* info, cast_function cast)
			: info(info)
			, cast(cast)
		{
		}
	};

	type_info const type_;
	std::vector<base_class_info> bases_;
	std::vector<class_info*> derivatives_;

	std::unordered_map<void*, persistent<v8::Object>> objects_;
};

template<typename T>
class class_singleton;

class class_singletons
{
public:
	template<typename T>
	static class_singleton<T>& add_class(v8::Isolate* isolate)
	{
		class_singletons* singletons = instance(add, isolate);
		type_info const type = type_id<T>();
		auto it = singletons->find(type);
		if (it != singletons->classes_.end())
		{
			//assert(false && "class already registred");
			throw std::runtime_error(class_name(type)
				+ " is already exist in isolate " + pointer_str(isolate));
		}
		singletons->classes_.emplace_back(new class_singleton<T>(isolate, type));
		return *static_cast<class_singleton<T>*>(singletons->classes_.back().get());
	}

	template<typename T>
	static void remove_class(v8::Isolate* isolate)
	{
		class_singletons* singletons = instance(get, isolate);
		if (singletons)
		{
			type_info const type = type_id<T>();
			auto it = singletons->find(type);
			if (it != singletons->classes_.end())
			{
				singletons->classes_.erase(it);
				if (singletons->classes_.empty())
				{
					instance(remove, isolate);
				}
			}
		}
	}

	template<typename T>
	static class_singleton<T>& find_class(v8::Isolate* isolate)
	{
		class_singletons* singletons = instance(get, isolate);
		type_info const type = type_id<T>();
		if (singletons)
		{
			auto it = singletons->find(type);
			if (it != singletons->classes_.end())
			{
				return *static_cast<class_singleton<T>*>(it->get());
			}
		}
		//assert(false && "class not registered");
		throw std::runtime_error(class_name(type)
			+ " not found in isolate " + pointer_str(isolate));
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
				return info->type() == type;
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

template<typename T>
class class_singleton : public class_info
{
public:
	class_singleton(v8::Isolate* isolate, type_info const& type)
		: class_info(type)
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
					class_singletons::find_class<T>(isolate).wrap_object(args));
			}
			catch (std::exception const& ex)
			{
				args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		});

		func_.Reset(isolate_, func);
		js_func_.Reset(isolate_, js_func);

		// each JavaScript instance has 2 internal fields:
		//  0 - pointer to a wrapped C++ object
		//  1 - pointer to the class_singleton
		func->InstanceTemplate()->SetInternalFieldCount(2);

		// no class constructor available by default
		ctor_ = nullptr;
		// use destructor from factory<T>::destroy()
		dtor_ = [](v8::Isolate* isolate, void* obj)
		{
			factory<T>::destroy(isolate, static_cast<T*>(obj));
		};

		func->Inherit(js_func);
	}

	class_singleton(class_singleton const&) = delete;
	class_singleton& operator=(class_singleton const&) = delete;

	~class_singleton()
	{
		destroy_objects();
	}

	v8::Handle<v8::Object> wrap(T* object, bool destroy_after)
	{
		v8::EscapableHandleScope scope(isolate_);

		v8::Local<v8::Object> obj = class_function_template()
                                            ->GetFunction()->NewInstance (v8::Isolate::GetCurrent()->GetCurrentContext())
                                            .FromMaybe(v8::Local<v8::Object>());
		obj->SetAlignedPointerInInternalField(0, object);
		obj->SetAlignedPointerInInternalField(1, this);

		persistent<v8::Object> pobj(isolate_, obj);
		if (destroy_after)
		{
			pobj.SetWeak(object,
#ifdef V8_USE_WEAK_CB_INFO
				[](v8::WeakCallbackInfo<T> const& data)
#else
				[](v8::WeakCallbackData<v8::Object, T> const& data)
#endif
			{
				v8::Isolate* isolate = data.GetIsolate();
				T* object = data.GetParameter();
				class_singletons::find_class<T>(isolate).destroy_object(object);
			}
#ifdef V8_USE_WEAK_CB_INFO
				, v8::WeakCallbackType::kParameter
#endif
				);
		}
		else
		{
			pobj.MarkIndependent();
			pobj.SetWeak(object,
#ifdef V8_USE_WEAK_CB_INFO
				[](v8::WeakCallbackInfo<T> const& data)
#else
				[](v8::WeakCallbackData<v8::Object, T> const& data)
#endif
			{
				v8::Isolate* isolate = data.GetIsolate();
				T* object = data.GetParameter();
				class_singletons::find_class<T>(isolate)
					.remove_object(isolate, object);
			}
#ifdef V8_USE_WEAK_CB_INFO
				, v8::WeakCallbackType::kParameter
#endif
				);

		}

		class_info::add_object(object, std::move(pobj));

		return scope.Escape(obj);
	}

	v8::Isolate* isolate() { return isolate_; }

	v8::Local<v8::FunctionTemplate> class_function_template()
	{
		return to_local(isolate_, func_);
	}

	v8::Local<v8::FunctionTemplate> js_function_template()
	{
		return to_local(isolate_, js_func_);
	}

	template<typename ...Args>
	void ctor()
	{
		ctor_ = [](v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			using ctor_type = T* (*)(v8::Isolate* isolate, Args...);
			return call_from_v8(static_cast<ctor_type>(&factory<T>::create), args);
		};
	}

	template<typename U>
	void inherit()
	{
		class_singleton<U>* base = &class_singletons::find_class<U>(isolate_);
		add_base(base, [](void* ptr) -> void*
			{
				return static_cast<U*>(static_cast<T*>(ptr));
			});
		js_function_template()->Inherit(base->class_function_template());
	}

	v8::Handle<v8::Object> wrap_external_object(T* object)
	{
		return wrap(object, false);
	}

	v8::Handle<v8::Object> wrap_object(T* object)
	{
		return wrap(object, true);
	}

	v8::Handle<v8::Object> wrap_object(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		if (!ctor_)
		{
			//assert(false && "create not allowed");
			throw std::runtime_error(class_name(type()) + " has no constructor");
		}
		return wrap_object(ctor_(args));
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
				class_info* info = static_cast<class_info*>(
					obj->GetAlignedPointerFromInternalField(1));
				if (info && info->cast(ptr, type()))
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
		class_info::remove_objects(isolate_, dtor_);
	}

	void destroy_object(T* obj)
	{
		class_info::remove_object(isolate_, obj, dtor_);
	}

private:
	v8::Isolate* isolate_;
	T* (*ctor_)(v8::FunctionCallbackInfo<v8::Value> const& args);
	void (*dtor_)(v8::Isolate*, void*);

	v8::UniquePersistent<v8::FunctionTemplate> func_;
	v8::UniquePersistent<v8::FunctionTemplate> js_func_;
};

} // namespace detail

/// Interface for registering C++ classes in V8
template<typename T>
class class_
{
	using class_singleton = detail::class_singleton<T>;
	detail::class_singleton<T>& class_singleton_;
public:
	explicit class_(v8::Isolate* isolate)
		: class_singleton_(detail::class_singletons::add_class<T>(isolate))
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
		static_assert(std::is_base_of<U, T>::value, "Class U should be base for class T");
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
			isolate(), name, wrap_function_template(isolate(),
				static_cast<mem_func_type>(mem_func)));
		return *this;
	}

	/// Set static class function
	template<typename Function,
		typename Func = typename std::decay<Function>::type>
	typename std::enable_if<detail::is_callable<Func>::value, class_&>::type
	set(char const *name, Function&& func)
	{
		v8::Local<v8::Data> wrapped_fun =
			wrap_function_template(isolate(), std::forward<Func>(func));
		class_singleton_.class_function_template()
			->PrototypeTemplate()->Set(isolate(), name, wrapped_fun);
		class_singleton_.js_function_template()->Set(isolate(), name, wrapped_fun);
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

	/// Set class attribute with getter and setter
	template<typename GetMethod, typename SetMethod>
	typename std::enable_if<std::is_member_function_pointer<GetMethod>::value
		&& std::is_member_function_pointer<SetMethod>::value, class_&>::type
	set(char const *name, property_<GetMethod, SetMethod>&& prop)
	{
		using prop_type = property_<GetMethod, SetMethod>;

		v8::HandleScope scope(isolate());

		using property_type = property_<
			typename detail::function_traits<GetMethod>::template pointer_type<T>,
			typename detail::function_traits<SetMethod>::template pointer_type<T>
		>;
		v8::AccessorGetterCallback getter = property_type::get;
		v8::AccessorSetterCallback setter = property_type::set;
		if (prop_type::is_readonly)
		{
			setter = nullptr;
		}

		class_singleton_.class_function_template()->PrototypeTemplate()
			->SetAccessor(v8pp::to_v8(isolate(), name),getter, setter,
				detail::set_external_data(isolate(),
					std::forward<prop_type>(prop)), v8::DEFAULT,
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
	static v8::Handle<v8::Object> reference_external(v8::Isolate* isolate, T* ext)
	{
		return detail::class_singletons::find_class<T>(isolate)
			.wrap_external_object(ext);
	}

	/// Remove external reference from JavaScript
	static void unreference_external(v8::Isolate* isolate, T* ext)
	{
		return detail::class_singletons::find_class<T>(isolate)
			.remove_object(isolate, ext);
	}

	/// As reference_external but delete memory for C++ object
	/// when JavaScript object is deleted. You must use "new" to allocate ext.
	static v8::Handle<v8::Object> import_external(v8::Isolate* isolate, T* ext)
	{
		return detail::class_singletons::find_class<T>(isolate)
			.wrap_object(ext);
	}

	/// Get wrapped object from V8 value, may return nullptr on fail.
	static T* unwrap_object(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		return detail::class_singletons::find_class<T>(isolate)
			.unwrap_object(value);
	}

	/// Create a wrapped C++ object and import it into JavaScript
	template<typename ...Args>
	static v8::Handle<v8::Object> create_object(v8::Isolate* isolate, Args... args)
	{
		T* obj = v8pp::factory<T>::create(isolate, std::forward<Args>(args)...);
		return import_external(isolate, obj);
	}

	/// Find V8 object handle for a wrapped C++ object, may return empty handle on fail.
	static v8::Handle<v8::Object> find_object(v8::Isolate* isolate, T const* obj)
	{
		return detail::class_singletons::find_class<T>(isolate)
			.find_object(obj);
	}

	/// Destroy wrapped C++ object
	static void destroy_object(v8::Isolate* isolate, T* obj)
	{
		detail::class_singletons::find_class<T>(isolate).destroy_object(obj);
	}

	/// Destroy all wrapped C++ objects of this class
	static void destroy_objects(v8::Isolate* isolate)
	{
		detail::class_singletons::find_class<T>(isolate).destroy_objects();
	}

	/// Destroy all wrapped C++ objects and this binding class_
	static void destroy(v8::Isolate* isolate)
	{
		detail::class_singletons::remove_class<T>(isolate);
	}

private:
	template<typename Attribute>
	static void member_get(v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		try
		{
			T const& self = v8pp::from_v8<T const&>(isolate, info.This());
			Attribute attr = detail::get_external_data<Attribute>(info.Data());
			info.GetReturnValue().Set(to_v8(isolate, self.*attr));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename Attribute>
	static void member_set(v8::Local<v8::String>, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		try
		{
			T& self = v8pp::from_v8<T&>(isolate, info.This());
			Attribute ptr = detail::get_external_data<Attribute>(info.Data());
			using attr_type = typename detail::function_traits<Attribute>::return_type;
			self.*ptr = v8pp::from_v8<attr_type>(isolate, value);
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}
};

inline void cleanup(v8::Isolate* isolate)
{
	detail::class_singletons::remove_all(isolate);
}

} // namespace v8pp

#endif // V8PP_CLASS_HPP_INCLUDED
