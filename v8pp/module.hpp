#pragma once

#include <v8.h>

#include "v8pp/function.hpp"
#include "v8pp/property.hpp"

namespace v8pp {

template<typename T, typename Traits>
class class_;

/// Module (similar to v8::ObjectTemplate)
class module
{
public:
	/// Create new module in the specified V8 isolate
	explicit module(v8::Isolate* isolate)
		: isolate_(isolate)
		, obj_(v8::ObjectTemplate::New(isolate))
	{
	}

	/// Create new module in the specified V8 isolate for existing ObjectTemplate
	explicit module(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> obj)
		: isolate_(isolate)
		, obj_(obj)
	{
	}

	module(module const&) = delete;
	module& operator=(module const&) = delete;

	module(module&&) = default;
	module& operator=(module&&) = default;

	/// v8::Isolate where the module belongs
	v8::Isolate* isolate() { return isolate_; }

	/// V8 ObjectTemplate implementation
	v8::Local<v8::ObjectTemplate> impl() const { return obj_; }

	/// Set a V8 value in the module with specified name
	template<typename Data>
	module& value(std::string_view name, v8::Local<Data> value)
	{
		obj_->Set(v8pp::to_v8(isolate_, name), value);
		return *this;
	}

	/// Set submodule in the module with specified name
	module& submodule(std::string_view name, v8pp::module& m)
	{
		return value(name, m.obj_);
	}

	/// Set wrapped C++ class in the module with specified name
	template<typename T, typename Traits>
	module& class_(std::string_view name, v8pp::class_<T, Traits>& cl)
	{
		v8::HandleScope scope(isolate_);

		cl.class_function_template()->SetClassName(v8pp::to_v8(isolate_, name));
		return value(name, cl.js_function_template());
	}

	/// Set a C++ function in the module with specified name
	template<typename Function, typename Traits = raw_ptr_traits>
	module& function(std::string_view name, Function&& func)
	{
		using Fun = typename std::decay_t<Function>;
		static_assert(detail::is_callable<Fun>::value, "Function must be callable");
		return value(name, wrap_function_template<Function, Traits>(isolate_, std::forward<Function>(func)));
	}

	/// Set a C++ variable in the module with specified name
	template<typename Variable>
	module& var(char const* name, Variable& var)
	{
		static_assert(!detail::is_callable<Variable>::value, "Variable must not be callable");
		v8::HandleScope scope(isolate_);

		v8::Local<v8::Name> v8_name = v8pp::to_v8(isolate_, name);
		v8::AccessorNameGetterCallback getter = &var_get<Variable>;
		v8::AccessorNameSetterCallback setter = &var_set<Variable>;
		v8::Local<v8::Value> data = detail::external_data::set(isolate_, &var);
		obj_->SetNativeDataProperty(v8_name, getter, setter, data, v8::PropertyAttribute::DontDelete);
		return *this;
	}

	/// Set property in the module with specified name and get/set functions
	template<typename GetFunction, typename SetFunction = detail::none>
	module& property(char const* name, GetFunction&& get, SetFunction&& set = {})
	{
		using Getter = typename std::decay_t<GetFunction>;
		using Setter = typename std::decay_t<SetFunction>;

		static_assert(detail::is_callable<Getter>::value, "GetFunction must be callable");
		static_assert(detail::is_callable<Setter>::value
			|| std::same_as<Setter, detail::none>, "SetFunction must be callable");

		using property_type = v8pp::property<Getter, Setter, detail::none, detail::none>;
		using Traits = detail::none;

		v8::HandleScope scope(isolate_);

		v8::Local<v8::Name> v8_name = v8pp::to_v8(isolate_, name);
		v8::AccessorNameGetterCallback getter = property_type::template get<Traits>;
		v8::AccessorNameSetterCallback setter = property_type::is_readonly ? nullptr : property_type::template set<Traits>;
		v8::Local<v8::Value> data = detail::external_data::set(isolate_, property_type(std::move(get), std::move(set)));
		obj_->SetNativeDataProperty(v8_name, getter, setter, data, v8::PropertyAttribute::DontDelete);
		return *this;
	}

	/// Set another module as a read-only property
	module& const_(std::string_view name, module& m)
	{
		v8::HandleScope scope(isolate_);

		obj_->Set(v8pp::to_v8(isolate_, name), m.obj_,
			v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	/// Set a value convertible to JavaScript as a read-only property
	template<typename Value>
	module& const_(std::string_view name, Value const& value)
	{
		v8::HandleScope scope(isolate_);

		obj_->Set(v8pp::to_v8(isolate_, name), to_v8(isolate_, value),
			v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	/// Create a new module instance in V8
	v8::Local<v8::Object> new_instance()
	{
		return obj_->NewInstance(isolate_->GetCurrentContext()).ToLocalChecked();
	}

private:
	template<typename Variable>
	static void var_get(v8::Local<v8::Name>, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Variable* var = detail::external_data::get<Variable*>(info.Data());
		info.GetReturnValue().Set(to_v8(isolate, *var));
	}

	template<typename Variable>
	static void var_set(v8::Local<v8::Name>, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Variable* var = detail::external_data::get<Variable*>(info.Data());
		*var = v8pp::from_v8<Variable>(isolate, value);
	}

	v8::Isolate* isolate_;
	v8::Local<v8::ObjectTemplate> obj_;
};

} // namespace v8pp
