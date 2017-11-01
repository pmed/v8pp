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

#include <algorithm>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <list>

#include "v8pp/config.hpp"
#include "v8pp/function.hpp"
#include "v8pp/property.hpp"
#include "v8pp/ptr_traits.hpp"

namespace v8pp {

namespace detail {

std::string pointer_str(void const* ptr);

struct class_info
{
	type_info const type;
	type_info const traits;

	class_info(type_info const& type, type_info const& traits);

	virtual ~class_info() = default; // make virtual to delete derived object_registry

	std::string class_name() const;
};

template<typename Traits>
class object_registry final : public class_info
{
public:
	using pointer_type = typename Traits::pointer_type;
	using const_pointer_type = typename Traits::const_pointer_type;
	using object_id = typename Traits::object_id;

	using ctor_function = pointer_type (*)(v8::FunctionCallbackInfo<v8::Value> const& args);
	using dtor_function = void         (*)(v8::Isolate*, pointer_type const&);
	using cast_function = pointer_type (*)(pointer_type const&);

	object_registry(v8::Isolate* isolate, type_info const& type, dtor_function dtor)
		: class_info(type, type_id<Traits>())
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

		// each JavaScript instance has 3 internal fields:
		//  0 - pointer to a wrapped C++ object
		//  1 - pointer to this object_registry
		//  2 - pointer to the object destructor
		func->InstanceTemplate()->SetInternalFieldCount(3);
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
		remove_objects();
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

	void remove_object(object_id const& obj)
	{
		auto it = objects_.find(Traits::key(obj));
		assert(it != objects_.end() && "no object");
		if (it != objects_.end())
		{
			v8::HandleScope scope(isolate_);
			reset_object(*it);
			objects_.erase(it);
		}
	}

	void remove_objects()
	{
		v8::HandleScope scope(isolate_);
		for (auto& object : objects_)
		{
			reset_object(object);
		}
		objects_.clear();
	}

	pointer_type find_object(object_id id, type_info const& type) const
	{
		auto it = objects_.find(Traits::key(id));
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

	v8::Local<v8::Object> wrap_object(pointer_type const& object, bool call_dtor)
	{
		auto it = objects_.find(object);
		if (it != objects_.end())
		{
			//assert(false && "duplicate object");
			throw std::runtime_error(class_name()
				+ " duplicate object " + pointer_str(Traits::pointer_id(object)));
		}

		v8::EscapableHandleScope scope(isolate_);

		v8::Local<v8::Context> context = isolate_->GetCurrentContext();
		v8::Local<v8::Object> obj = class_function_template()
			->GetFunction(context).ToLocalChecked()->NewInstance(context).ToLocalChecked();

		obj->SetAlignedPointerInInternalField(0, Traits::pointer_id(object));
		obj->SetAlignedPointerInInternalField(1, this);
		obj->SetAlignedPointerInInternalField(2, call_dtor? /*dtor_*/this : nullptr);

		v8::Global<v8::Object> pobj(isolate_, obj);
		pobj.SetWeak(this, [](v8::WeakCallbackInfo<object_registry> const& data)
		{
			object_id object = data.GetInternalField(0);
			object_registry* this_ = static_cast<object_registry*>(data.GetInternalField(1));
			this_->remove_object(object);
		}, v8::WeakCallbackType::kInternalFields);
		objects_.emplace(object, std::move(pobj));

		return scope.Escape(obj);
	}

	v8::Local<v8::Object> wrap_object(v8::FunctionCallbackInfo<v8::Value> const& args)
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
			v8::Local<v8::Object> obj = value.As<v8::Object>();
			if (obj->InternalFieldCount() == 3)
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
	void reset_object(std::pair<pointer_type const, v8::Global<v8::Object>>& object)
	{
		bool call_dtor = true;
		if (!object.second.IsNearDeath())
		{
			v8::Local<v8::Object> obj = to_local(isolate_, object.second);
			assert(obj->InternalFieldCount() == 3);
			assert(obj->GetAlignedPointerFromInternalField(0) == Traits::pointer_id(object.first));
			assert(obj->GetAlignedPointerFromInternalField(1) == this);
			call_dtor = (obj->GetAlignedPointerFromInternalField(2) != nullptr);
			// remove internal fields to disable unwrapping for this V8 Object
			obj->SetAlignedPointerInInternalField(0, nullptr);
			obj->SetAlignedPointerInInternalField(1, nullptr);
			obj->SetAlignedPointerInInternalField(2, nullptr);
		}
		if (call_dtor)
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

	std::unordered_map<pointer_type, v8::Global<v8::Object>> objects_;

	v8::Isolate* isolate_;
	v8::Global<v8::FunctionTemplate> func_;
	v8::Global<v8::FunctionTemplate> js_func_;

	ctor_function ctor_;
	dtor_function dtor_;
};

class classes
{
public:
	template<typename Traits>
	static object_registry<Traits>& add(v8::Isolate* isolate, type_info const& type,
		typename object_registry<Traits>::dtor_function dtor)
	{
		classes* info = instance(operation::add, isolate);
		auto it = info->find(type);
		if (it != info->classes_.end())
		{
			//assert(false && "class already registred");
			throw std::runtime_error((*it)->class_name()
				+ " is already exist in isolate " + pointer_str(isolate));
		}
		info->classes_.emplace_back(new object_registry<Traits>(isolate, type, dtor));
		return *static_cast<object_registry<Traits>*>(info->classes_.back().get());
	}

	template<typename Traits>
	static void remove(v8::Isolate* isolate, type_info const& type)
	{
		classes* info = instance(operation::get, isolate);
		if (info)
		{
			auto it = info->find(type);
			if (it != info->classes_.end())
			{
				type_info const& traits = type_id<Traits>();
				if ((*it)->traits != traits)
				{
					throw std::runtime_error((*it)->class_name()
						+ " is already registered in isolate "
						+ pointer_str(isolate) + " before of "
						+ class_info(type, traits).class_name());
				}
				info->classes_.erase(it);
				if (info->classes_.empty())
				{
					instance(operation::remove, isolate);
				}
			}
		}
	}

	template<typename Traits>
	static object_registry<Traits>& find(v8::Isolate* isolate, type_info const& type)
	{
		classes* info = instance(operation::get, isolate);
		type_info const& traits = type_id<Traits>();
		if (info)
		{
			auto it = info->find(type);
			if (it != info->classes_.end())
			{
				if ((*it)->traits != traits)
				{
					throw std::runtime_error((*it)->class_name()
						+ " is already registered in isolate "
						+ pointer_str(isolate) + " before of "
						+ class_info(type, traits).class_name());
				}
				return *static_cast<object_registry<Traits>*>(it->get());
			}
		}
		//assert(false && "class not registered");
		throw std::runtime_error(class_info(type, traits).class_name()
			+ " is not registered in isolate " + pointer_str(isolate));
	}

	static void remove_all(v8::Isolate* isolate);

private:
	using classes_info = std::vector<std::unique_ptr<class_info>>;
	classes_info classes_;

	classes_info::iterator find(type_info const& type);

	enum class operation { get, add, remove };
	static classes* instance(operation op, v8::Isolate* isolate);
};

} // namespace detail

/// Interface to access C++ classes bound to V8
template<typename T, typename Traits = raw_ptr_traits>
class class_
{
	static_assert(is_wrapped_class<T>::value, "T must be a user-defined class");

	using object_registry = detail::object_registry<Traits>;
	object_registry& class_info_;

	using object_id = typename object_registry::object_id;
	using pointer_type = typename object_registry::pointer_type;
	using const_pointer_type = typename object_registry::const_pointer_type;

public:
	using object_pointer_type = typename Traits::template object_pointer_type<T>;
	using object_const_pointer_type = typename Traits::template object_const_pointer_type<T>;

	using ctor_function = object_pointer_type(*)(v8::FunctionCallbackInfo<v8::Value> const& args);
	using dtor_function = void(*)(v8::Isolate* isolate, pointer_type const& obj);

	template<typename ...Args>
	static object_pointer_type object_create(v8::Isolate* isolate, Args&&...args)
	{
		object_pointer_type object = Traits::template create<T>(std::forward<Args>(args)...);
		isolate->AdjustAmountOfExternalAllocatedMemory(
			static_cast<int64_t>(Traits::object_size(object)));
		return object;
	}

	template<typename ...Args>
	struct object_create_from_v8
	{
		static object_pointer_type call(v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			using ctor_function = object_pointer_type (*)(v8::Isolate* isolate, Args&&...);
			object_pointer_type object = detail::call_from_v8<Traits>(Traits::template create<T, Args...>, args);
			args.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(
				static_cast<int64_t>(Traits::object_size(object)));
			return object;
		}
	};

	static void object_destroy(v8::Isolate* isolate, pointer_type const& ptr)
	{
		object_pointer_type object = Traits::template static_pointer_cast<T>(ptr);
		Traits::destroy(object);
		isolate->AdjustAmountOfExternalAllocatedMemory(
			-static_cast<int64_t>(Traits::object_size(object)));
	}

public:
	explicit class_(v8::Isolate* isolate, dtor_function destroy = object_destroy)
		: class_info_(detail::classes::add<Traits>(isolate, detail::type_id<T>(), destroy))
	{
	}

	/// Set class constructor signature
	template<typename ...Args, typename Create = object_create_from_v8<Args...>>
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
		object_registry& base = classes::find<Traits>(isolate(), type_id<U>());
		class_info_.add_base(base, [](pointer_type const& ptr) -> pointer_type
		{
			return pointer_type(Traits::template static_pointer_cast<U>(
				Traits::template static_pointer_cast<T>(ptr)));
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
				&detail::forward_function<Traits, mem_func_type>,
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
		v8::AccessorGetterCallback getter = property_type::template get<Traits>;
		v8::AccessorSetterCallback setter = property_type::template set<Traits>;
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
	static v8::Local<v8::Object> reference_external(v8::Isolate* isolate,
		object_pointer_type const& ext)
	{
		using namespace detail;
		return classes::find<Traits>(isolate, type_id<T>()).wrap_object(ext, false);
	}

	/// Remove external reference from JavaScript
	static void unreference_external(v8::Isolate* isolate, object_pointer_type const& ext)
	{
		using namespace detail;
		return classes::find<Traits>(isolate, type_id<T>()).remove_object(Traits::pointer_id(ext));
	}

	/// As reference_external but delete memory for C++ object
	/// when JavaScript object is deleted. You must use `Traits::create<T>()`
	/// to allocate `ext`
	static v8::Local<v8::Object> import_external(v8::Isolate* isolate, object_pointer_type const& ext)
	{
		using namespace detail;
		return classes::find<Traits>(isolate, type_id<T>()).wrap_object(ext, true);
	}

	/// Get wrapped object from V8 value, may return nullptr on fail.
	static object_pointer_type unwrap_object(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		using namespace detail;
		return Traits::template static_pointer_cast<T>(
			classes::find<Traits>(isolate, type_id<T>()).unwrap_object(value));
	}

	/// Create a wrapped C++ object and import it into JavaScript
	template<typename ...Args>
	static v8::Local<v8::Object> create_object(v8::Isolate* isolate, Args&&... args)
	{
		return import_external(isolate, object_create(isolate, std::forward<Args>(args)...));
	}

	/// Find V8 object handle for a wrapped C++ object, may return empty handle on fail.
	static v8::Local<v8::Object> find_object(v8::Isolate* isolate,
		object_const_pointer_type const& obj)
	{
		using namespace detail;
		return classes::find<Traits>(isolate, type_id<T>())
			.find_v8_object(Traits::const_pointer_cast(obj));
	}

	/// Destroy wrapped C++ object
	static void destroy_object(v8::Isolate* isolate, object_pointer_type const& obj)
	{
		using namespace detail;
		classes::find<Traits>(isolate, type_id<T>()).remove_object(Traits::pointer_id(obj));
	}

	/// Destroy all wrapped C++ objects of this class
	static void destroy_objects(v8::Isolate* isolate)
	{
		using namespace detail;
		classes::find<Traits>(isolate, type_id<T>()).remove_objects();
	}

	/// Destroy all wrapped C++ objects and this binding class_
	static void destroy(v8::Isolate* isolate)
	{
		using namespace detail;
		classes::remove<Traits>(isolate, type_id<T>());
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
using shared_class = class_<T, shared_ptr_traits>;

void cleanup(v8::Isolate* isolate);

} // namespace v8pp

#if V8PP_HEADER_ONLY
#include "v8pp/class.ipp"
#endif

#endif // V8PP_CLASS_HPP_INCLUDED
