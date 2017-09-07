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

template<typename T, typename U>
T* static_pointer_cast(U* ptr)
{
	return static_cast<T*>(ptr);
}

using std::static_pointer_cast;
using std::const_pointer_cast;

struct class_info
{
	type_info const type;
	bool const is_shared_ptr;

	class_info(type_info const& type, bool is_shared_ptr)
		: type(type)
		, is_shared_ptr(is_shared_ptr)
	{
	}

	virtual ~class_info() = default; // make virtual to delete derived object_registry

	std::string class_name() const
	{
		return "v8pp::class_<" + type.name()
			+ (is_shared_ptr ? ", use_shared_ptr>" : ">");
	}
};

template<bool use_shared_ptr>
class object_registry final : public class_info
{
public:
	using pointer_type = typename std::conditional<use_shared_ptr,
		std::shared_ptr<void>, void*>::type;
	using const_pointer_type = typename std::conditional<use_shared_ptr,
		std::shared_ptr<void const>, void const*>::type;

	using object_id = void*; //TODO: make opaque type

	using ctor_function = pointer_type (*)(v8::FunctionCallbackInfo<v8::Value> const& args);
	using dtor_function = void         (*)(v8::Isolate*, pointer_type const&);
	using cast_function = pointer_type (*)(pointer_type const&);

	object_registry(v8::Isolate* isolate, type_info const& type, dtor_function dtor)
		: class_info(type, use_shared_ptr)
		, isolate_(isolate)
		, ctor_(nullptr) // no wrapped class constructor available by default
		, dtor_(dtor)
	{
		v8::HandleScope scope(isolate_);

		v8::Local<v8::FunctionTemplate> func = v8::FunctionTemplate::New(isolate_);
		v8::Local<v8::FunctionTemplate> js_func = v8::FunctionTemplate::New(isolate_,
			[](v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			v8::Isolate* isolate = args.GetIsolate();
			object_registry* this_ = get_external_data<object_registry*>(args.Data());
			try
			{
				return args.GetReturnValue().Set(this_->wrap_object(args));
			}
			catch (std::exception const& ex)
			{
				args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}, set_external_data(isolate, this));

		func_.Reset(isolate, func);
		js_func_.Reset(isolate, js_func);

		// each JavaScript instance has 2 internal fields:
		//  0 - pointer to a wrapped C++ object
		//  1 - pointer to this object_registry
		func->InstanceTemplate()->SetInternalFieldCount(2);
		func->Inherit(js_func);
	}

	object_registry(object_registry const&) = delete;
	object_registry(object_registry && src) // = default; Visual C++ can't do this: error C2610
		: class_info(std::move(static_cast<class_info&&>(src)))
		, bases_(std::move(src.bases_))
		, derivatives_(std::move(src.derivatives_))
		, objects_(std::move(src.objects_))
		, isolate_(std::move(src.isolate_))
		, func_(std::move(src.func_))
		, js_func_(std::move(src.js_func_))
		, ctor_(std::move(src.ctor_))
		, dtor_(std::move(src.dtor_))
	{
	}

	object_registry& operator=(object_registry const&) = delete;
	object_registry& operator=(object_registry &&) = delete;

	~object_registry()
	{
		remove_objects(true);
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

	void set_ctor(ctor_function ctor) { ctor_ = ctor; }

	void add_base(object_registry& info, cast_function cast)
	{
		auto it = std::find_if(bases_.begin(), bases_.end(),
			[&info](base_class_info const& base) { return &base.info == &info; });
		if (it != bases_.end())
		{
			//assert(false && "duplicated inheritance");
			throw std::runtime_error(class_name()
				+ " is already inherited from " + info.class_name());
		}
		bases_.emplace_back(info, cast);
		info.derivatives_.emplace_back(this);
	}

	bool cast(pointer_type& ptr, type_info const& type) const
	{
		if (this->type == type || !ptr)
		{
			return true;
		}

		// fast way - search a direct parent
		for (base_class_info const& base : bases_)
		{
			if (base.info.type == type)
			{
				ptr = base.cast(ptr);
				return true;
			}
		}

		// slower way - walk on hierarhy
		for (base_class_info const& base : bases_)
		{
			pointer_type p = base.cast(ptr);
			if (base.info.cast(p, type))
			{
				ptr = p;
				return true;
			}
		}
		return false;
	}

	void remove_object(object_id const& obj, bool call_dtor)
	{
		auto it = objects_.find(key(obj, std::integral_constant<bool, use_shared_ptr>{}));
		assert(it != objects_.end() && "no object");
		if (it != objects_.end())
		{
			v8::HandleScope scope(isolate_);
			reset_object(*it, call_dtor);
			objects_.erase(it);
		}
	}

	void remove_objects(bool call_dtor)
	{
		v8::HandleScope scope(isolate_);
		for (auto& object : objects_)
		{
			reset_object(object, call_dtor);
		}
		objects_.clear();
	}

	pointer_type find_object(object_id id, type_info const& type) const
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

	v8::Local<v8::Object> find_v8_object(pointer_type const& ptr) const
	{
		auto it = objects_.find(ptr);
		if (it != objects_.end())
		{
			return to_local(isolate_, it->second);
		}

		v8::Local<v8::Object> result;
		for (auto const info : derivatives_)
		{
			result = info->find_v8_object(ptr);
			if (!result.IsEmpty()) break;
		}
		return result;
	}

	v8::Handle<v8::Object> wrap_object(pointer_type const& object, bool call_dtor)
	{
		auto it = objects_.find(object);
		if (it != objects_.end())
		{
			//assert(false && "duplicate object");
			throw std::runtime_error(class_name()
				+ " duplicate object " + pointer_str(pointer_id(object)));
		}

		v8::EscapableHandleScope scope(isolate_);

		v8::Local<v8::Object> obj = class_function_template()
			->GetFunction()->NewInstance();

		obj->SetAlignedPointerInInternalField(0, pointer_id(object));
		obj->SetAlignedPointerInInternalField(1, this);

		v8::UniquePersistent<v8::Object> pobj(isolate_, obj);
		pobj.SetWeak(call_dtor? this : nullptr,
			[](v8::WeakCallbackInfo<object_registry> const& data)
		{
			bool const call_dtor = (data.GetParameter() != nullptr);
			object_id object = data.GetInternalField(0);
			object_registry* this_ = static_cast<object_registry*>(data.GetInternalField(1));
			this_->remove_object(object, call_dtor);
		}, v8::WeakCallbackType::kInternalFields);
		if (!call_dtor)
		{
			pobj.MarkIndependent();
		}
		objects_.emplace(object, std::move(pobj));

		return scope.Escape(obj);
	}

	v8::Handle<v8::Object> wrap_object(v8::FunctionCallbackInfo<v8::Value> const& args)
	{
		if (!ctor_)
		{
			//assert(false && "create not allowed");
			throw std::runtime_error(class_name() + " has no constructor");
		}
		return wrap_object(ctor_(args), true);
	}

	pointer_type unwrap_object(v8::Local<v8::Value> value)
	{
		v8::HandleScope scope(isolate_);

		while (value->IsObject())
		{
			v8::Handle<v8::Object> obj = value->ToObject();
			if (obj->InternalFieldCount() == 2)
			{
				object_id id = obj->GetAlignedPointerFromInternalField(0);
				if (id)
				{
					auto registry = static_cast<object_registry*>(
						obj->GetAlignedPointerFromInternalField(1));
					if (registry)
					{
						pointer_type ptr = registry->find_object(id, type);
						if (ptr)
						{
							return ptr;
						}
					}
				}
			}
			value = obj->GetPrototype();
		}
		return nullptr;
	}

private:
	void reset_object(std::pair<pointer_type const, v8::UniquePersistent<v8::Object>>& object, bool call_dtor)
	{
		if (!object.second.IsNearDeath())
		{
			v8::Local<v8::Object> obj = to_local(isolate_, object.second);
			assert(obj->InternalFieldCount() == 2);
			assert(obj->GetAlignedPointerFromInternalField(0) == pointer_id(object.first));
			assert(obj->GetAlignedPointerFromInternalField(1) == this);
			// remove internal fields to disable unwrapping for this V8 Object
			obj->SetAlignedPointerInInternalField(0, nullptr);
			obj->SetAlignedPointerInInternalField(1, nullptr);
		}
		if (call_dtor && !object.second.IsIndependent())
		{
			dtor_(isolate_, object.first);
		}
		object.second.Reset();
	}

	struct base_class_info
	{
		object_registry& info;
		cast_function cast;

		base_class_info(object_registry& info, cast_function cast)
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

	std::unordered_map<pointer_type, v8::UniquePersistent<v8::Object>> objects_;

	v8::Isolate* isolate_;
	v8::UniquePersistent<v8::FunctionTemplate> func_;
	v8::UniquePersistent<v8::FunctionTemplate> js_func_;

	ctor_function ctor_;
	dtor_function dtor_;
};

class classes
{
public:
	template<bool use_shared_ptr>
	static object_registry<use_shared_ptr>& add(v8::Isolate* isolate, type_info const& type,
		typename object_registry<use_shared_ptr>::dtor_function dtor)
	{
		classes* info = instance(operation::add, isolate);
		auto it = info->find(type);
		if (it != info->classes_.end())
		{
			//assert(false && "class already registred");
			throw std::runtime_error(class_info(type, use_shared_ptr).class_name()
				+ " is already exist in isolate " + pointer_str(isolate));
		}
		info->classes_.emplace_back(new object_registry<use_shared_ptr>(isolate, type, dtor));
		return *static_cast<object_registry<use_shared_ptr>*>(info->classes_.back().get());
	}

	template<bool use_shared_ptr>
	static void remove(v8::Isolate* isolate, type_info const& type)
	{
		classes* info = instance(operation::get, isolate);
		if (info)
		{
			auto it = info->find(type);
			if (it != info->classes_.end())
			{
				if ((*it)->is_shared_ptr != use_shared_ptr)
				{
					throw std::runtime_error((*it)->class_name()
						+ " is already registered in isolate "
						+ pointer_str(isolate) + " before of "
						+ class_info(type, use_shared_ptr).class_name());
				}
				info->classes_.erase(it);
				if (info->classes_.empty())
				{
					instance(operation::remove, isolate);
				}
			}
		}
	}

	template<bool use_shared_ptr>
	static object_registry<use_shared_ptr>& find(v8::Isolate* isolate, type_info const& type)
	{
		classes* info = instance(operation::get, isolate);
		if (info)
		{
			auto it = info->find(type);
			if (it != info->classes_.end())
			{
				if ((*it)->is_shared_ptr != use_shared_ptr)
				{
					throw std::runtime_error((*it)->class_name()
						+ " is already registered in isolate "
						+ pointer_str(isolate) + " before of "
						+ class_info(type, use_shared_ptr).class_name());
				}
				return *static_cast<object_registry<use_shared_ptr>*>(it->get());
			}
		}
		//assert(false && "class not registered");
		throw std::runtime_error(class_info(type, use_shared_ptr).class_name()
			+ " is not registered in isolate " + pointer_str(isolate));
	}

	static void remove_all(v8::Isolate* isolate)
	{
		instance(operation::remove, isolate);
	}

private:
	using classes_info = std::vector<std::unique_ptr<class_info>>;
	classes_info classes_;

	classes_info::iterator find(type_info const& type)
	{
		return std::find_if(classes_.begin(), classes_.end(),
			[&type](classes_info::value_type const& info)
			{
				return info->type == type;
			});
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

/// Interface to access C++ classes bound to V8
template<typename T, bool use_shared_ptr = false>
class class_
{
	static_assert(is_wrapped_class<T>::value, "T must be a user-defined class");

	using object_registry = detail::object_registry<use_shared_ptr>;
	object_registry& class_info_;

	using object_id = typename object_registry::object_id;
	using pointer_type = typename object_registry::pointer_type;
	using const_pointer_type = typename object_registry::const_pointer_type;

public:
	using object_pointer_type = typename std::conditional<use_shared_ptr,
		std::shared_ptr<T>, T*>::type;
	using object_const_pointer_type = typename std::conditional<use_shared_ptr,
		std::shared_ptr<T const>, T const*>::type;

	template<typename ...Args>
	struct factory_create
	{
		static object_pointer_type call(v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			using ctor_function = object_pointer_type(*)(v8::Isolate* isolate, Args...);
			return detail::call_from_v8(static_cast<ctor_function>(
				&factory<T, use_shared_ptr>::create), args);
		}
	};

	static void factory_destroy(v8::Isolate* isolate, pointer_type const& obj)
	{
		factory<T, use_shared_ptr>::destroy(isolate, detail::static_pointer_cast<T>(obj));
	}

	using ctor_function = object_pointer_type (*)(v8::FunctionCallbackInfo<v8::Value> const& args);
	using dtor_function = void (*)(v8::Isolate* isolate, pointer_type const& obj);

public:
	explicit class_(v8::Isolate* isolate, dtor_function destroy = factory_destroy)
		: class_info_(detail::classes::add<use_shared_ptr>(isolate, detail::type_id<T>(), destroy))
	{
	}

	/// Set class constructor signature
	template<typename ...Args, typename Create = factory_create<Args...>>
	class_& ctor(ctor_function create = Create::call)
	{
		class_info_.set_ctor(reinterpret_cast<typename object_registry::ctor_function>(create));
		return *this;
	}

	/// Inhert from C++ class U
	template<typename U>
	class_& inherit()
	{
		using namespace detail;
		static_assert(std::is_base_of<U, T>::value,
			"Class U should be base for class T");
		//TODO: std::is_convertible<T*, U*> and check for duplicates in hierarchy?
		object_registry& base = classes::find<use_shared_ptr>(isolate(), type_id<U>());
		class_info_.add_base(base, [](pointer_type const& ptr) -> pointer_type
		{
			return pointer_type(static_pointer_cast<U>(
				static_pointer_cast<T>(ptr)));
		});
		class_info_.js_function_template()->Inherit(base.class_function_template());
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
		mem_func_type mf(mem_func);
		class_info_.class_function_template()->PrototypeTemplate()->Set(
			isolate(), name, v8::FunctionTemplate::New(isolate(),
				&detail::forward_function<mem_func_type, use_shared_ptr>,
				detail::set_external_data(isolate(), std::forward<mem_func_type>(mf))));
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
		attribute_type attr(attribute);
		v8::AccessorGetterCallback getter = &member_get<attribute_type>;
		v8::AccessorSetterCallback setter = &member_set<attribute_type>;
		if (readonly)
		{
			setter = nullptr;
		}

		class_info_.class_function_template()->PrototypeTemplate()
			->SetAccessor(v8pp::to_v8(isolate(), name), getter, setter,
				detail::set_external_data(isolate(),
					std::forward<attribute_type>(attr)), v8::DEFAULT,
				v8::PropertyAttribute(v8::DontDelete | (setter? 0 : v8::ReadOnly)));
		return *this;
	}

	/// Set read/write class property with getter and setter
	template<typename GetMethod, typename SetMethod>
	typename std::enable_if<std::is_member_function_pointer<GetMethod>::value
		&& std::is_member_function_pointer<SetMethod>::value, class_&>::type
	set(char const *name, property_<GetMethod, SetMethod>&& property)
	{
		v8::HandleScope scope(isolate());

		using property_type = property_<
			typename detail::function_traits<GetMethod>::template pointer_type<T>,
			typename detail::function_traits<SetMethod>::template pointer_type<T>
		>;
		property_type prop(property);
		v8::AccessorGetterCallback getter = property_type::template get<use_shared_ptr>;
		v8::AccessorSetterCallback setter = property_type::template set<use_shared_ptr>;
		if (prop.is_readonly)
		{
			setter = nullptr;
		}

		class_info_.class_function_template()->PrototypeTemplate()
			->SetAccessor(v8pp::to_v8(isolate(), name), getter, setter,
				detail::set_external_data(isolate(),
					std::forward<property_type>(prop)), v8::DEFAULT,
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
	static v8::Handle<v8::Object> reference_external(v8::Isolate* isolate,
		object_pointer_type const& ext)
	{
		using namespace detail;
		return classes::find<use_shared_ptr>(isolate, type_id<T>()).wrap_object(ext, false);
	}

	/// Remove external reference from JavaScript
	static void unreference_external(v8::Isolate* isolate, object_pointer_type const& ext)
	{
		using namespace detail;
		return classes::find<use_shared_ptr>(isolate, type_id<T>()).remove_object(pointer_id(ext), false);
	}

	/// As reference_external but delete memory for C++ object
	/// when JavaScript object is deleted. You must use `factory<T>::create()`
	/// to allocate `ext`
	static v8::Handle<v8::Object> import_external(v8::Isolate* isolate, object_pointer_type const& ext)
	{
		using namespace detail;
		return classes::find<use_shared_ptr>(isolate, type_id<T>()).wrap_object(ext, true);
	}

	/// Get wrapped object from V8 value, may return nullptr on fail.
	static object_pointer_type unwrap_object(v8::Isolate* isolate,
		v8::Handle<v8::Value> value)
	{
		using namespace detail;
		return static_pointer_cast<T>(
			classes::find<use_shared_ptr>(isolate, type_id<T>()).unwrap_object(value));
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
		using namespace detail;
		return classes::find<use_shared_ptr>(isolate, type_id<T>())
			.find_v8_object(const_pointer_cast(obj));
	}

	/// Destroy wrapped C++ object
	static void destroy_object(v8::Isolate* isolate, object_pointer_type const& obj)
	{
		using namespace detail;
		classes::find<use_shared_ptr>(isolate, type_id<T>()).remove_object(pointer_id(obj), true);
	}

	/// Destroy all wrapped C++ objects of this class
	static void destroy_objects(v8::Isolate* isolate)
	{
		using namespace detail;
		classes::find<use_shared_ptr>(isolate, type_id<T>()).remove_objects(true);
	}

	/// Destroy all wrapped C++ objects and this binding class_
	static void destroy(v8::Isolate* isolate)
	{
		using namespace detail;
		classes::remove<use_shared_ptr>(isolate, type_id<T>());
	}

private:
	template<typename Attribute>
	static void member_get(v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		try
		{
			auto self = unwrap_object(isolate, info.This());
			Attribute attr = detail::get_external_data<Attribute>(info.Data());
			info.GetReturnValue().Set(to_v8(isolate, (*self).*attr));
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
			auto self = unwrap_object(isolate, info.This());
			Attribute ptr = detail::get_external_data<Attribute>(info.Data());
			using attr_type = typename detail::function_traits<Attribute>::return_type;
			(*self).*ptr = v8pp::from_v8<attr_type>(isolate, value);
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}
};

/// Interface to access C++ classes bound to V8
/// Objects are stored in std::shared_ptr
template<typename T>
using shared_class = class_<T, true>;

inline void cleanup(v8::Isolate* isolate)
{
	detail::classes::remove_all(isolate);
}

} // namespace v8pp

#endif // V8PP_CLASS_HPP_INCLUDED
