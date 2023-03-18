#ifndef V8PP_CLASS_HPP_INCLUDED
#define V8PP_CLASS_HPP_INCLUDED

#include <algorithm>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "v8pp/config.hpp"
#include "v8pp/function.hpp"
#include "v8pp/property.hpp"
#include "v8pp/ptr_traits.hpp"

namespace v8pp {

namespace detail {

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

	using ctor_function = std::function<pointer_type (v8::FunctionCallbackInfo<v8::Value> const& args)>;
	using dtor_function = std::function<void (v8::Isolate*, pointer_type const&)>;
	using cast_function = pointer_type (*)(pointer_type const&);

	object_registry(v8::Isolate* isolate, type_info const& type, dtor_function&& dtor);

	object_registry(object_registry const&) = delete;
	object_registry(object_registry&&) = default;

	object_registry& operator=(object_registry const&) = delete;
	object_registry& operator=(object_registry&&) = delete;

	~object_registry();

	v8::Isolate* isolate() { return isolate_; }

	v8::Local<v8::FunctionTemplate> class_function_template()
	{
		return to_local(isolate_, func_);
	}

	v8::Local<v8::FunctionTemplate> js_function_template()
	{
		return to_local(isolate_, js_func_);
	}

	void set_auto_wrap_objects(bool auto_wrap) { auto_wrap_objects_ = auto_wrap; }
	bool auto_wrap_objects() const { return auto_wrap_objects_; }

	void set_ctor(ctor_function&& ctor) { ctor_ = std::move(ctor); }

	void add_base(object_registry& info, cast_function cast);
	bool cast(pointer_type& ptr, type_info const& actual_type) const;

	void remove_object(object_id const& obj);
	void remove_objects();

	pointer_type find_object(object_id id, type_info const& actual_type) const;
	v8::Local<v8::Object> find_v8_object(pointer_type const& ptr) const;

	v8::Local<v8::Object> wrap_object(pointer_type const& object, bool call_dtor);
	v8::Local<v8::Object> wrap_object(v8::FunctionCallbackInfo<v8::Value> const& args);
	pointer_type unwrap_object(v8::Local<v8::Value> value);

private:
	struct wrapped_object
	{
		v8::Global<v8::Object> pobj;
		bool call_dtor;
	};

	void reset_object(pointer_type const& object, wrapped_object& wrapped);

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
	std::unordered_map<pointer_type, wrapped_object> objects_;

	v8::Isolate* isolate_;
	v8::Global<v8::FunctionTemplate> func_;
	v8::Global<v8::FunctionTemplate> js_func_;

	ctor_function ctor_;
	dtor_function dtor_;
	bool auto_wrap_objects_;
};

class classes
{
public:
	template<typename Traits>
	static object_registry<Traits>& add(v8::Isolate* isolate, type_info const& type,
		typename object_registry<Traits>::dtor_function&& dtor);

	template<typename Traits>
	static void remove(v8::Isolate* isolate, type_info const& type);

	template<typename Traits>
	static object_registry<Traits>& find(v8::Isolate* isolate, type_info const& type);

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

	using ctor_function = std::function<object_pointer_type(v8::FunctionCallbackInfo<v8::Value> const& args)>;
	using dtor_function = std::function<void(v8::Isolate* isolate, object_pointer_type const& obj)>;

private:
	template<typename... Args>
	static object_pointer_type object_create(v8::Isolate* isolate, Args&&... args)
	{
		object_pointer_type object = Traits::template create<T>(std::forward<Args>(args)...);
		isolate->AdjustAmountOfExternalAllocatedMemory(
			static_cast<int64_t>(Traits::object_size(object)));
		return object;
	}

	template<typename... Args>
	struct object_create_from_v8
	{
		static object_pointer_type call(v8::FunctionCallbackInfo<v8::Value> const& args)
		{
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

	explicit class_(v8::Isolate* isolate, detail::type_info const& existing)
		: class_info_(detail::classes::find<Traits>(isolate, existing))
	{
	}

public:
	explicit class_(v8::Isolate* isolate, dtor_function destroy = &object_destroy)
		: class_info_(detail::classes::add<Traits>(isolate, detail::type_id<T>(),
			[destroy = std::move(destroy)](v8::Isolate* isolate, pointer_type const& obj)
			{
				destroy(isolate, Traits::template static_pointer_cast<T>(obj));
			}))
	{
	}

	class_(class_ const&) = delete;
	class_& operator=(class_ const&) = delete;

	class_(class_&&) = default;
	class_& operator=(class_&&) = delete;

	/// Find existing class_ to extend bindings
	static class_ extend(v8::Isolate* isolate)
	{
		return class_(isolate, detail::type_id<T>());
	}

	/// Set class constructor signature
	template<typename... Args, typename Create = object_create_from_v8<Args...>>
	class_& ctor(ctor_function create = &Create::call)
	{
		class_info_.set_ctor([create = std::move(create)](v8::FunctionCallbackInfo<v8::Value> const& args)
		{
			return create(args);
		});
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

	/// Enable new C++ objects auto-wrapping
	class_& auto_wrap_objects(bool auto_wrap = true)
	{
		class_info_.set_auto_wrap_objects(auto_wrap);
		return *this;
	}

	/// Set class member function, or static function, or lambda
	template<typename Function>
	class_& function(std::string_view name, Function&& func, v8::PropertyAttribute attr = v8::None)
	{
		constexpr bool is_mem_fun = std::is_member_function_pointer_v<Function>;

		static_assert(is_mem_fun || detail::is_callable<Function>::value,
			"Function must be pointer to member function or callable object");

		v8::HandleScope scope(isolate());

		v8::Local<v8::Name> v8_name = v8pp::to_v8(isolate(), name);
		v8::Local<v8::Data> wrapped_fun;

		if constexpr (is_mem_fun)
		{
			using mem_func_type = typename detail::function_traits<Function>::template pointer_type<T>;
			wrapped_fun = wrap_function_template<mem_func_type, Traits>(isolate(), mem_func_type(std::forward<Function>(func)));
		}
		else
		{
			wrapped_fun = wrap_function_template<Function, Traits>(isolate(), std::forward<Function>(func));
			class_info_.js_function_template()->Set(v8_name, wrapped_fun, attr);
		}

		class_info_.class_function_template()->PrototypeTemplate()->Set(v8_name, wrapped_fun, attr);
		return *this;
	}

	/// Set class member variable
	template<typename Attribute>
	class_& var(std::string_view name, Attribute attribute)
	{
		static_assert(std::is_member_object_pointer<Attribute>::value,
			"Attribute must be pointer to member data");

		v8::HandleScope scope(isolate());

		using attribute_type = typename detail::function_traits<Attribute>::template pointer_type<T>;
		attribute_type attr = attribute;

		v8::Local<v8::Name> v8_name = v8pp::to_v8(isolate(), name);
		v8::Local<v8::Value> data = detail::external_data::set(isolate(), std::forward<attribute_type>(attr));
		class_info_.class_function_template()->PrototypeTemplate()
			->SetAccessor(v8_name,
				&member_get<attribute_type>, &member_set<attribute_type>,
				data,
				v8::DEFAULT, v8::PropertyAttribute(v8::DontDelete));
		return *this;
	}

	/// Set read/write class property with getter and setter
	template<typename GetFunction, typename SetFunction = detail::none>
	class_& property(std::string_view name, GetFunction&& get, SetFunction&& set = {})
	{
		using Getter = typename std::conditional<
			std::is_member_function_pointer<GetFunction>::value,
			typename detail::function_traits<GetFunction>::template pointer_type<T>,
			typename std::decay<GetFunction>::type>::type;

		using Setter = typename std::conditional<
			std::is_member_function_pointer<SetFunction>::value,
			typename detail::function_traits<SetFunction>::template pointer_type<T>,
			typename std::decay<SetFunction>::type>::type;

		static_assert(std::is_member_function_pointer<GetFunction>::value
			|| detail::is_callable<Getter>::value, "GetFunction must be callable");
		static_assert(std::is_member_function_pointer<SetFunction>::value
			|| detail::is_callable<Setter>::value
			|| std::is_same<Setter, detail::none>::value, "SetFunction must be callable");

		using GetClass = std::conditional_t<detail::function_with_object<Getter, T>, T, detail::none>;
		using SetClass = std::conditional_t<detail::function_with_object<Setter, T>, T, detail::none>;

		using property_type = v8pp::property<Getter, Setter, GetClass, SetClass>;

		v8::HandleScope scope(isolate());

		v8::AccessorGetterCallback getter = property_type::template get<Traits>;
		v8::AccessorSetterCallback setter = property_type::is_readonly ? nullptr : property_type::template set<Traits>;
		v8::Local<v8::String> v8_name = v8pp::to_v8(isolate(), name);
		v8::Local<v8::Value> data = detail::external_data::set(isolate(), property_type(std::move(get), std::move(set)));
		class_info_.class_function_template()->PrototypeTemplate()
			->SetAccessor(v8_name, getter, setter, data, v8::DEFAULT, v8::PropertyAttribute(v8::DontDelete));
		return *this;
	}

	/// Set value as a read-only constant
	template<typename Value>
	class_& const_(std::string_view name, Value const& value)
	{
		v8::HandleScope scope(isolate());

		class_info_.class_function_template()->PrototypeTemplate()
			->Set(v8pp::to_v8(isolate(), name), to_v8(isolate(), value),
				v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	/// Set value as a class static property
	template<typename Value>
	class_& static_(std::string_view const& name, Value const& value, bool readonly = false)
	{
		v8::HandleScope scope(isolate());

		class_info_.js_function_template()->GetFunction(isolate()->GetCurrentContext()).ToLocalChecked()
			->DefineOwnProperty(isolate()->GetCurrentContext(),
				v8pp::to_v8(isolate(), name), to_v8(isolate(), value),
				v8::PropertyAttribute(v8::DontDelete | (readonly ? v8::ReadOnly : 0))).FromJust();
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
	template<typename... Args>
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

	/// Find V8 object handle for a wrapped C++ object, may return empty handle on fail
	/// or wrap a copy of the obj if class_.auto_wrap_objects()
	static v8::Local<v8::Object> find_object(v8::Isolate* isolate, T const& obj)
	{
		using namespace detail;
		detail::object_registry<Traits>& class_info = classes::find<Traits>(isolate, type_id<T>());
		v8::Local<v8::Object> wrapped_object = class_info.find_v8_object(Traits::key(const_cast<T*>(&obj)));
		if (wrapped_object.IsEmpty() && class_info.auto_wrap_objects())
		{
			object_pointer_type clone = Traits::clone(obj);
			if (clone)
			{
				wrapped_object = class_info.wrap_object(clone, true);
			}
		}
		return wrapped_object;
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
	static void member_get(v8::Local<v8::Name>,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		try
		{
			auto self = unwrap_object(isolate, info.This());
			Attribute attr = detail::external_data::get<Attribute>(info.Data());
			info.GetReturnValue().Set(to_v8(isolate, (*self).*attr));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename Attribute>
	static void member_set(v8::Local<v8::Name>, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		try
		{
			auto self = unwrap_object(isolate, info.This());
			Attribute ptr = detail::external_data::get<Attribute>(info.Data());
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
