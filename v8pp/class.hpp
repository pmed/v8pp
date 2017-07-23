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
#include <list>

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
	using ctor_function = void* (*)(v8::FunctionCallbackInfo<v8::Value> const& args);
	using dtor_function = void  (*)(v8::Isolate*, void* obj);
	using cast_function = void* (*)(void* ptr);

	class_info(v8::Isolate* isolate, type_info const& type, dtor_function dtor)
		: isolate_(isolate)
		, type_(type)
		, ctor_(nullptr) // no wrapped class constructor available by default
		, dtor_(dtor)
	{
		v8::HandleScope scope(isolate_);

		v8::Local<v8::FunctionTemplate> func = v8::FunctionTemplate::New(isolate_);
		v8::Local<v8::FunctionTemplate> js_func = v8::FunctionTemplate::New(isolate_,
			[](v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			v8::Isolate* isolate = args.GetIsolate();
			class_info* this_ = get_external_data<class_info*>(args.Data());
			try
			{
				return args.GetReturnValue().Set(this_->wrap_object(args));
			}
			catch (std::exception const& ex)
			{
				args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}, set_external_data(isolate_, this));

		func_.Reset(isolate_, func);
		js_func_.Reset(isolate_, js_func);

		// each JavaScript instance has 2 internal fields:
		//  0 - pointer to a wrapped C++ object
		//  1 - pointer to this class_info
		func->InstanceTemplate()->SetInternalFieldCount(2);
		func->Inherit(js_func);
	}

	class_info(class_info const&) = delete;
	class_info(class_info && src) // = default; Visual C++ can't do this: error C2610
		: isolate_(std::move(src.isolate_))
		, type_(std::move(src.type_))
		, bases_(std::move(src.bases_))
		, derivatives_(std::move(src.derivatives_))
		, objects_(std::move(src.objects_))
		, func_(std::move(src.func_))
		, js_func_(std::move(src.js_func_))
		, ctor_(std::move(src.ctor_))
		, dtor_(std::move(src.dtor_))
	{
	}

	class_info& operator=(class_info const&) = delete;
	class_info& operator=(class_info &&) = delete;

	~class_info()
	{
		destroy_objects();
	}

	v8::Isolate* isolate() const { return isolate_; }

	v8::Local<v8::FunctionTemplate> class_function_template()
	{
		return to_local(isolate_, func_);
	}

	v8::Local<v8::FunctionTemplate> js_function_template()
	{
		return to_local(isolate_, js_func_);
	}

	type_info const& type() const { return type_; }

	void add_base(class_info& info, cast_function cast)
	{
		auto it = std::find_if(bases_.begin(), bases_.end(),
			[&info](base_class_info const& base) { return &base.info == &info; });
		if (it != bases_.end())
		{
			//assert(false && "duplicated inheritance");
			throw std::runtime_error(class_name(type_)
				+ " is already inherited from " + class_name(info.type_));
		}
		bases_.emplace_back(info, cast);
		info.derivatives_.emplace_back(this);
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
			if (base.info.type_ == type)
			{
				ptr = base.cast(ptr);
				return true;
			}
		}

		// slower way - walk on hierarhy
		for (base_class_info const& base : bases_)
		{
			void* p = base.cast(ptr);
			if (base.info.cast(p, type))
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

	void remove_object(void* object,
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
				assert(to_local(isolate_, it->second)
					->GetAlignedPointerFromInternalField(0) == object);
				to_local(isolate_, it->second)
					->SetAlignedPointerInInternalField(0, nullptr);
			}
			if (destroy && !it->second.IsIndependent())
			{
				destroy(isolate_, object);
			}
			it->second.Reset();
			objects_.erase(it);
		}
	}

	void remove_objects(void (*destroy)(v8::Isolate* isolate, void* obj))
	{
		for (auto& object : objects_)
		{
			if (destroy && !object.second.IsIndependent())
			{
				destroy(isolate_, object.first);
			}
			object.second.Reset();
		}
		objects_.clear();
	}

	void destroy_objects()
	{
		remove_objects(dtor_);
	}

	void destroy_object(void* obj)
	{
		remove_object(obj, dtor_);
	}

	v8::Local<v8::Object> find_object(void const* object) const
	{
		auto it = objects_.find(const_cast<void*>(object));
		if (it != objects_.end())
		{
			return to_local(isolate_, it->second);
		}

		v8::Local<v8::Object> result;
		for (class_info const* info : derivatives_)
		{
			result = info->find_object(object);
			if (!result.IsEmpty()) break;
		}
		return result;
	}

	v8::Handle<v8::Object> wrap(void* object, bool destroy_after)
	{
		v8::EscapableHandleScope scope(isolate_);

		v8::Local<v8::Object> obj = class_function_template()
			->GetFunction()->NewInstance();
		obj->SetAlignedPointerInInternalField(0, object);
		obj->SetAlignedPointerInInternalField(1, this);

		persistent<v8::Object> pobj(isolate(), obj);
		if (destroy_after)
		{
			pobj.SetWeak(this,
				[](v8::WeakCallbackInfo<class_info> const& data)
			{
				class_info* this_ = data.GetParameter();
				this_->destroy_object(data.GetInternalField(0));
			}, v8::WeakCallbackType::kInternalFields);
		}
		else
		{
			pobj.MarkIndependent();
			pobj.SetWeak(this,
				[](v8::WeakCallbackInfo<class_info> const& data)
			{
				class_info* this_ = data.GetParameter();
				this_->remove_object(data.GetInternalField(0));
			}, v8::WeakCallbackType::kInternalFields);
		}

		class_info::add_object(object, std::move(pobj));

		return scope.Escape(obj);
	}

	template<typename T, typename ...Args>
	void ctor()
	{
		ctor_ = [](v8::FunctionCallbackInfo<v8::Value> const& args) -> void*
		{
			using ctor_type = T* (*)(v8::Isolate* isolate, Args...);
			return call_from_v8(static_cast<ctor_type>(&factory<T>::create), args);
		};
	}

	template<typename T, typename U>
	void inherit()
	{
		class_info& base = classes::find(isolate_, type_id<U>());
		add_base(base, [](void* ptr) -> void*
		{
			return static_cast<U*>(static_cast<T*>(ptr));
		});
		js_function_template()->Inherit(base.class_function_template());
	}

	v8::Handle<v8::Object> wrap_external_object(void* object)
	{
		return wrap(object, false);
	}

	v8::Handle<v8::Object> wrap_object(void* object)
	{
		return wrap(object, true);
	}

	v8::Handle<v8::Object> wrap_object(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		if (!ctor_)
		{
			//assert(false && "create not allowed");
			throw std::runtime_error(class_name(type_) + " has no constructor");
		}
		return wrap_object(ctor_(args));
	}

	void* unwrap_object(v8::Local<v8::Value> value)
	{
		v8::HandleScope scope(isolate());

		while (value->IsObject())
		{
			v8::Handle<v8::Object> obj = value->ToObject();
			if (obj->InternalFieldCount() == 2)
			{
				void* ptr = obj->GetAlignedPointerFromInternalField(0);
				class_info* info = static_cast<class_info*>(
					obj->GetAlignedPointerFromInternalField(1));
				if (info && info->cast(ptr, type_))
				{
					return ptr;
				}
			}
			value = obj->GetPrototype();
		}
		return nullptr;
	}

private:
	struct base_class_info
	{
		class_info& info;
		cast_function cast;

		base_class_info(class_info& info, cast_function cast)
			: info(info)
			, cast(cast)
		{
		}
	};

	v8::Isolate* isolate_;
	type_info type_;
	std::vector<base_class_info> bases_;
	std::vector<class_info*> derivatives_;

	std::unordered_map<void*, v8::UniquePersistent<v8::Object>> objects_;

	v8::UniquePersistent<v8::FunctionTemplate> func_;
	v8::UniquePersistent<v8::FunctionTemplate> js_func_;

	ctor_function ctor_;
	dtor_function dtor_;
};

class classes
{
public:
	static class_info& add(v8::Isolate* isolate, type_info const& type,
		class_info::dtor_function dtor)
	{
		classes* info = instance(operation::add, isolate);
		auto it = info->find(type);
		if (it != info->classes_.end())
		{
			//assert(false && "class already registred");
			throw std::runtime_error(class_name(type)
				+ " is already exist in isolate " + pointer_str(isolate));
		}
		info->classes_.emplace_back(isolate, type, dtor);
		return info->classes_.back();
	}

	static void remove(v8::Isolate* isolate, type_info const& type)
	{
		classes* info = instance(operation::get, isolate);
		if (info)
		{
			auto it = info->find(type);
			if (it != info->classes_.end())
			{
				info->classes_.erase(it);
				if (info->classes_.empty())
				{
					instance(operation::remove, isolate);
				}
			}
		}
	}

	static class_info& find(v8::Isolate* isolate, type_info const& type)
	{
		classes* info = instance(operation::get, isolate);
		if (info)
		{
			auto it = info->find(type);
			if (it != info->classes_.end())
			{
				return *it;
			}
		}
		//assert(false && "class not registered");
		throw std::runtime_error(class_name(type)
			+ " not found in isolate " + pointer_str(isolate));
	}

	static void remove_all(v8::Isolate* isolate)
	{
		instance(operation::remove, isolate);
	}

private:
	std::list<class_info> classes_;

	std::list<class_info>::iterator find(type_info const& type)
	{
		return std::find_if(classes_.begin(), classes_.end(),
			[&type](class_info const& info) { return info.type() == type; });
	}

	enum class operation { get, add, remove };
	static classes* instance(operation op, v8::Isolate* isolate)
	{
#if defined(V8PP_ISOLATE_DATA_SLOT)
		classes* info = static_cast<classes*>(
			isolate->GetData(V8PP_ISOLATE_DATA_SLOT));
		switch (op)
		{
		case operation::get:
			return info;
		case operation::add:
			if (!info)
			{
				info = new classes;
				isolate->SetData(V8PP_ISOLATE_DATA_SLOT, info);
			}
			return info;
		case operation::remove:
			if (info)
			{
				delete info;
				isolate->SetData(V8PP_ISOLATE_DATA_SLOT, nullptr);
			}
		default:
			return nullptr;
		}
#else
		static std::unordered_map<v8::Isolate*, classes> instances;
		switch (op)
		{
		case operation::get:
			{
				auto it = instances.find(isolate);
				return it != instances.end()? &it->second : nullptr;
			}
		case operation::add:
			return &instances[isolate];
		case operation::remove:
			instances.erase(isolate);
		default:
			return nullptr;
		}
#endif
	}
};

} // namespace detail

/// Interface for registering C++ classes in V8
template<typename T>
class class_
{
	detail::class_info& class_info_;

	static void dtor(v8::Isolate* isolate, void* obj)
	{
		factory<T>::destroy(isolate, static_cast<T*>(obj));
	};

public:
	explicit class_(v8::Isolate* isolate)
		: class_info_(detail::classes::add(isolate, detail::type_id<T>(), dtor))
	{
	}

	/// Set class constructor signature
	template<typename ...Args>
	class_& ctor()
	{
		class_info_.template ctor<T, Args...>();
		return *this;
	}

	/// Inhert from C++ class U
	template<typename U>
	class_& inherit()
	{
		static_assert(std::is_base_of<U, T>::value, "Class U should be base for class T");
		//TODO: std::is_convertible<T*, U*> and check for duplicates in hierarchy?
		class_info_.template inherit<T, U>();
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

		class_info_.class_function_template()->PrototypeTemplate()->Set(
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
		class_info_.class_function_template()
			->PrototypeTemplate()->Set(isolate(), name, wrapped_fun);
		class_info_.js_function_template()->Set(isolate(), name, wrapped_fun);
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

		class_info_.class_function_template()->PrototypeTemplate()
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

		class_info_.class_function_template()->PrototypeTemplate()
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

		class_info_.class_function_template()->PrototypeTemplate()
			->Set(v8pp::to_v8(isolate(), name), to_v8(isolate(), value),
				v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	/// v8::Isolate where the class bindings belongs
	v8::Isolate* isolate() { return class_info_.isolate(); }

	v8::Local<v8::FunctionTemplate> class_function_template()
	{
		return class_info_.class_function_template();
	}

	v8::Local<v8::FunctionTemplate> js_function_template()
	{
		return class_info_.js_function_template();
	}

	/// Create JavaScript object which references externally created C++ class.
	/// It will not take ownership of the C++ pointer.
	static v8::Handle<v8::Object> reference_external(v8::Isolate* isolate, T* ext)
	{
		return detail::classes::find(isolate, detail::type_id<T>()).wrap_external_object(ext);
	}

	/// Remove external reference from JavaScript
	static void unreference_external(v8::Isolate* isolate, T* ext)
	{
		return detail::classes::find(isolate, detail::type_id<T>())
			.remove_object(ext);
	}

	/// As reference_external but delete memory for C++ object
	/// when JavaScript object is deleted. You must use "new" to allocate ext.
	static v8::Handle<v8::Object> import_external(v8::Isolate* isolate, T* ext)
	{
		return detail::classes::find(isolate, detail::type_id<T>())
			.wrap_object(ext);
	}

	/// Get wrapped object from V8 value, may return nullptr on fail.
	static T* unwrap_object(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		return static_cast<T*>(detail::classes::find(isolate, detail::type_id<T>())
			.unwrap_object(value));
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
		return detail::classes::find(isolate, detail::type_id<T>()).find_object(obj);
	}

	/// Destroy wrapped C++ object
	static void destroy_object(v8::Isolate* isolate, T* obj)
	{
		detail::classes::find(isolate, detail::type_id<T>()).destroy_object(obj);
	}

	/// Destroy all wrapped C++ objects of this class
	static void destroy_objects(v8::Isolate* isolate)
	{
		detail::classes::find(isolate, detail::type_id<T>()).destroy_objects();
	}

	/// Destroy all wrapped C++ objects and this binding class_
	static void destroy(v8::Isolate* isolate)
	{
		detail::classes::remove(isolate, detail::type_id<T>());
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
	detail::classes::remove_all(isolate);
}

} // namespace v8pp

#endif // V8PP_CLASS_HPP_INCLUDED
