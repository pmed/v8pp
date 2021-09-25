#include "v8pp/class.hpp"

#include <cstdio> // for snprintf

namespace v8pp {

namespace detail {

static V8PP_IMPL std::string pointer_str(void const* ptr)
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

/////////////////////////////////////////////////////////////////////////////
//
// class_info
//
V8PP_IMPL class_info::class_info(type_info const& type, type_info const& traits)
	: type(type)
	, traits(traits)
{
}

V8PP_IMPL std::string class_info::class_name() const
{
	return "v8pp::class_<" + std::string(type.name()) + ", " + std::string(traits.name()) + ">";
}

/////////////////////////////////////////////////////////////////////////////
//
// object_registry
//
template<typename Traits>
V8PP_IMPL object_registry<Traits>::object_registry(v8::Isolate* isolate, type_info const& type, dtor_function&& dtor)
	: class_info(type, type_id<Traits>())
	, isolate_(isolate)
	, ctor_() // no wrapped class constructor available by default
	, dtor_(std::move(dtor))
	, auto_wrap_objects_(false)
{
	v8::HandleScope scope(isolate_);

	v8::Local<v8::FunctionTemplate> func = v8::FunctionTemplate::New(isolate_);
	v8::Local<v8::FunctionTemplate> js_func = v8::FunctionTemplate::New(isolate_,
		[](v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			v8::Isolate* isolate = args.GetIsolate();
			object_registry* this_ = external_data::get<object_registry*>(args.Data());
			try
			{
				return args.GetReturnValue().Set(this_->wrap_object(args));
			}
			catch (std::exception const& ex)
			{
				args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
			}
		}, external_data::set(isolate, this));

	func_.Reset(isolate, func);
	js_func_.Reset(isolate, js_func);

	// each JavaScript instance has 3 internal fields:
	//  0 - pointer to a wrapped C++ object
	//  1 - pointer to this object_registry
	func->InstanceTemplate()->SetInternalFieldCount(2);
	func->Inherit(js_func);
}

template<typename Traits>
V8PP_IMPL object_registry<Traits>::~object_registry()
{
	remove_objects();
}

template<typename Traits>
V8PP_IMPL void object_registry<Traits>::add_base(object_registry& info, cast_function cast)
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

template<typename Traits>
V8PP_IMPL bool object_registry<Traits>::cast(pointer_type& ptr, type_info const& type) const
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

template<typename Traits>
V8PP_IMPL void object_registry<Traits>::remove_object(object_id const& obj)
{
	auto it = objects_.find(Traits::key(obj));
	assert(it != objects_.end() && "no object");
	if (it != objects_.end())
	{
		v8::HandleScope scope(isolate_);
		reset_object(it->first, it->second);
		objects_.erase(it);
	}
}

template<typename Traits>
V8PP_IMPL void object_registry<Traits>::remove_objects()
{
	v8::HandleScope scope(isolate_);
	for (auto& object_wrapped : objects_)
	{
		reset_object(object_wrapped.first, object_wrapped.second);
	}
	objects_.clear();
}

template<typename Traits>
V8PP_IMPL typename object_registry<Traits>::pointer_type
object_registry<Traits>::find_object(object_id id, type_info const& type) const
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

template<typename Traits>
V8PP_IMPL v8::Local<v8::Object> object_registry<Traits>::find_v8_object(pointer_type const& ptr) const
{
	auto it = objects_.find(ptr);
	if (it != objects_.end())
	{
		return to_local(isolate_, it->second.pobj);
	}

	v8::Local<v8::Object> result;
	for (auto const info : derivatives_)
	{
		result = info->find_v8_object(ptr);
		if (!result.IsEmpty()) break;
	}
	return result;
}

template<typename Traits>
V8PP_IMPL v8::Local<v8::Object> object_registry<Traits>::wrap_object(pointer_type const& object, bool call_dtor)
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

	v8::Global<v8::Object> pobj(isolate_, obj);
	pobj.SetWeak(this, [](v8::WeakCallbackInfo<object_registry> const& data)
		{
			object_id object = data.GetInternalField(0);
			object_registry* this_ = static_cast<object_registry*>(data.GetInternalField(1));
			this_->remove_object(object);
		}, v8::WeakCallbackType::kInternalFields);
	objects_.emplace(object, wrapped_object{ std::move(pobj), call_dtor });

	return scope.Escape(obj);
}

template<typename Traits>
V8PP_IMPL v8::Local<v8::Object> object_registry<Traits>::wrap_object(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	if (!ctor_)
	{
		//assert(false && "create not allowed");
		throw std::runtime_error(class_name() + " has no constructor");
	}
	return wrap_object(ctor_(args), true);
}

template<typename Traits>
V8PP_IMPL typename object_registry<Traits>::pointer_type
object_registry<Traits>::unwrap_object(v8::Local<v8::Value> value)
{
	v8::HandleScope scope(isolate_);

	while (value->IsObject())
	{
		v8::Local<v8::Object> obj = value.As<v8::Object>();
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

template<typename Traits>
V8PP_IMPL void object_registry<Traits>::reset_object(pointer_type const& object, wrapped_object& wrapped)
{
	if (wrapped.call_dtor)
	{
		dtor_(isolate_, object);
	}
	wrapped.pobj.Reset();
}

/////////////////////////////////////////////////////////////////////////////
//
// classes
//
template<typename Traits>
V8PP_IMPL object_registry<Traits>& classes::add(v8::Isolate* isolate, type_info const& type,
	typename object_registry<Traits>::dtor_function&& dtor)
{
	classes* info = instance(operation::add, isolate);
	auto it = info->find(type);
	if (it != info->classes_.end())
	{
		//assert(false && "class already registred");
		throw std::runtime_error((*it)->class_name()
			+ " is already exist in isolate " + pointer_str(isolate));
	}
	info->classes_.emplace_back(new object_registry<Traits>(isolate, type, std::move(dtor)));
	return *static_cast<object_registry<Traits>*>(info->classes_.back().get());
}

template<typename Traits>
V8PP_IMPL void classes::remove(v8::Isolate* isolate, type_info const& type)
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
V8PP_IMPL object_registry<Traits>& classes::find(v8::Isolate* isolate, type_info const& type)
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

V8PP_IMPL void classes::remove_all(v8::Isolate* isolate)
{
	instance(operation::remove, isolate);
}

V8PP_IMPL classes::classes_info::iterator classes::find(type_info const& type)
{
	return std::find_if(classes_.begin(), classes_.end(),
		[&type](classes_info::value_type const& info)
		{
			return info->type == type;
		});
}

V8PP_IMPL classes* classes::instance(operation op, v8::Isolate* isolate)
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
		// fallthrough
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
			return it != instances.end() ? &it->second : nullptr;
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

} // namespace detail

V8PP_IMPL void cleanup(v8::Isolate* isolate)
{
	detail::classes::remove_all(isolate);
	detail::external_data::destroy_all(isolate);
}

} // namespace v8pp
