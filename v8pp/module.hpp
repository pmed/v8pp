#ifndef V8PP_MODULE_HPP_INCLUDED
#define V8PP_MODULE_HPP_INCLUDED

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
	module& set(string_view name, v8::Local<Data> value)
	{
		obj_->Set(v8pp::to_v8(isolate_, name), value);
		return *this;
	}

	/// Set submodule in the module with specified name
	module& set(string_view name, v8pp::module& m)
	{
		return set(name, m.obj_);
	}

	/// Set wrapped C++ class in the module with specified name
	template<typename T, typename Traits>
	module& set(string_view name, v8pp::class_<T, Traits>& cl)
	{
		v8::HandleScope scope(isolate_);

		cl.class_function_template()->SetClassName(v8pp::to_v8(isolate_, name));
		return set(name, cl.js_function_template());
	}

	/// Set a C++ function in the module with specified name
	template<typename Function, typename Fun = typename std::decay<Function>::type>
	typename std::enable_if<detail::is_callable<Fun>::value, module&>::type
	set(string_view name, Function&& func)
	{
		return set(name, wrap_function_template(isolate_, std::forward<Fun>(func)));
	}

	/// Set a C++ variable in the module with specified name
	template<typename Variable>
	typename std::enable_if<!detail::is_callable<Variable>::value, module&>::type
	set(string_view name, Variable& var, bool readonly = false)
	{
		v8::HandleScope scope(isolate_);

		v8::AccessorGetterCallback getter = &var_get<Variable>;
		v8::AccessorSetterCallback setter = &var_set<Variable>;
		if (readonly)
		{
			setter = nullptr;
		}

		obj_->SetAccessor(v8pp::to_v8(isolate_, name), getter, setter,
			detail::external_data::set(isolate_, &var),
			v8::DEFAULT,
			v8::PropertyAttribute(v8::DontDelete | (setter ? 0 : v8::ReadOnly)));
		return *this;
	}

	/// Set v8pp::property in the module with specified name
	template<typename GetFunction, typename SetFunction>
	module& set(string_view name, property_<GetFunction, SetFunction>&& property)
	{
		using property_type = property_<GetFunction, SetFunction>;

		v8::HandleScope scope(isolate_);

		v8::AccessorGetterCallback getter = property_type::get;
		v8::AccessorSetterCallback setter = property_type::set;
		if (property_type::is_readonly)
		{
			setter = nullptr;
		}

		obj_->SetAccessor(v8pp::to_v8(isolate_, name), getter, setter,
			detail::external_data::set(isolate_, std::forward<property_type>(property)),
			v8::DEFAULT,
			v8::PropertyAttribute(v8::DontDelete | (setter ? 0 : v8::ReadOnly)));
		return *this;
	}

	/// Set another module as a read-only property
	module& set_const(string_view name, module& m)
	{
		v8::HandleScope scope(isolate_);

		obj_->Set(v8pp::to_v8(isolate_, name), m.obj_,
			v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
		return *this;
	}

	/// Set a value convertible to JavaScript as a read-only property
	template<typename Value>
	module& set_const(string_view name, Value const& value)
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
	static void var_get(v8::Local<v8::String>,
		v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Variable* var = detail::external_data::get<Variable*>(info.Data());
		info.GetReturnValue().Set(to_v8(isolate, *var));
	}

	template<typename Variable>
	static void var_set(v8::Local<v8::String>, v8::Local<v8::Value> value,
		v8::PropertyCallbackInfo<void> const& info)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Variable* var = detail::external_data::get<Variable*>(info.Data());
		*var = v8pp::from_v8<Variable>(isolate, value);
	}

	v8::Isolate* isolate_;
	v8::Local<v8::ObjectTemplate> obj_;
};

} // namespace v8pp

#endif // V8PP_MODULE_HPP_INCLUDED
