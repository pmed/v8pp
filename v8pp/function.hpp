#ifndef V8PP_FUNCTION_HPP_INCLUDED
#define V8PP_FUNCTION_HPP_INCLUDED

#include <tuple>
#include <type_traits>

#include "v8pp/call_from_v8.hpp"
#include "v8pp/ptr_traits.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/utility.hpp"

namespace v8pp {

namespace detail {

class external_data
{
	template<typename T>
	using is_pointer_cast_allowed = std::integral_constant<bool, sizeof(T) <= sizeof(void*)
		&& std::is_trivial<T>::value>;

	template<typename T>
	union pointer_cast
	{
	private:
		void* ptr;
		T value;

	public:
		static_assert(is_pointer_cast_allowed<T>::value, "pointer_cast is not allowed");

		explicit pointer_cast(void* ptr) : ptr(ptr) {}
		explicit pointer_cast(T value) : value(value) {}

		operator void*() const { return ptr; }
		operator T() const { return value; }
	};

public:
	template<typename T>
	static typename std::enable_if<is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
	set(v8::Isolate* isolate, T value)
	{
		return v8::External::New(isolate, pointer_cast<T>(value));
	}

	template<typename T>
	static typename std::enable_if<!is_pointer_cast_allowed<T>::value, v8::Local<v8::Value>>::type
	set(v8::Isolate* isolate, T&& data)
	{
		value_holder<T>* value = new value_holder<T>(isolate, std::forward<T>(data));
		return v8::Local<v8::External>::New(isolate, value->pext);
	}

	template<typename T>
	static typename std::enable_if<is_pointer_cast_allowed<T>::value, T>::type
	get(v8::Local<v8::Value> value)
	{
		return pointer_cast<T>(value.As<v8::External>()->Value());
	}

	template<typename T>
	static typename std::enable_if<!is_pointer_cast_allowed<T>::value, T&>::type
	get(v8::Local<v8::Value> ext)
	{
		value_holder<T>* value = static_cast<value_holder<T>*>(ext.As<v8::External>()->Value());
		return value->data();
	}

	static void destroy_all(v8::Isolate* isolate)
	{
		handle_visitor visitor(isolate);
		isolate->VisitHandlesWithClassIds(&visitor);
	}

private:
	static constexpr uint16_t class_id = 0x7bc;

	struct value_holder_base
	{
		virtual ~value_holder_base() = default;
	};

	template<typename T>
	struct value_holder final : value_holder_base
	{
		typename std::aligned_storage<sizeof(T)>::type storage;
		v8::Global<v8::External> pext;

		T& data() { return *static_cast<T*>(static_cast<void*>(&storage)); }

		value_holder(v8::Isolate* isolate, T&& data)
		{
			new (&storage) T(std::forward<T>(data));
			pext.Reset(isolate, v8::External::New(isolate, this));
			pext.SetWrapperClassId(external_data::class_id);
			pext.SetWeak(this,
				[](v8::WeakCallbackInfo<value_holder> const& info)
				{
					delete info.GetParameter();
				}, v8::WeakCallbackType::kParameter);
		}

		~value_holder()
		{
			if (!pext.IsEmpty())
			{
				data().~T();
				pext.Reset();
			}
		}
	};

	struct handle_visitor final : v8::PersistentHandleVisitor
	{
		v8::Isolate* isolate;

		explicit handle_visitor(v8::Isolate* isolate)
			: isolate(isolate)
		{
		}

		virtual void VisitPersistentHandle(v8::Persistent<v8::Value>* value, uint16_t value_class_id) override
		{
			if (value_class_id == external_data::class_id)
			{
				v8::HandleScope scope(isolate);
				v8::Local<v8::External> ext = value->Get(isolate).As<v8::External>();
				if (!ext.IsEmpty())
				{
					delete static_cast<value_holder_base*>(ext->Value());
				}
			}
		}
	};
};

template<typename Traits, typename F>
typename function_traits<F>::return_type
invoke(v8::FunctionCallbackInfo<v8::Value> const& args, std::false_type /*is_member_function_pointer*/)
{
	return call_from_v8<Traits, F>(std::forward<F>(external_data::get<F>(args.Data())), args);
}

template<typename Traits, typename F>
typename function_traits<F>::return_type
invoke(v8::FunctionCallbackInfo<v8::Value> const& args, std::true_type /*is_member_function_pointer*/)
{
	using arguments = typename function_traits<F>::arguments;
	static_assert(std::tuple_size<arguments>::value > 0, "");
	using class_type = typename std::decay<
		typename std::tuple_element<0, arguments>::type>::type;

	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Object> obj = args.This();
	auto ptr = class_<class_type, Traits>::unwrap_object(isolate, obj);
	if (!ptr)
	{
		throw std::runtime_error("method called on null instance");
	}
	return call_from_v8<Traits, class_type, F>(*ptr, std::forward<F>(external_data::get<F>(args.Data())), args);
}

template<typename Traits, typename F>
void forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, std::true_type /*is_void_return*/)
{
	invoke<Traits, F>(args, std::is_member_function_pointer<F>());
}

template<typename Traits, typename F>
void forward_ret(v8::FunctionCallbackInfo<v8::Value> const& args, std::false_type /*is_void_return*/)
{
	using return_type = typename function_traits<F>::return_type;
	using converter = typename call_from_v8_traits<F>::template arg_converter<return_type, Traits>;
	args.GetReturnValue().Set(converter::to_v8(args.GetIsolate(),
		invoke<Traits, F>(args, std::is_member_function_pointer<F>())));
}

template<typename Traits, typename F>
void forward_function(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	static_assert(is_callable<F>::value || std::is_member_function_pointer<F>::value,
		"required callable F");

	v8::Isolate* isolate = args.GetIsolate();
	v8::HandleScope scope(isolate);

	try
	{
		forward_ret<Traits, F>(args, is_void_return<F>());
	}
	catch (std::exception const& ex)
	{
		args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
	}
}

} // namespace detail

/// Wrap C++ function into new V8 function template
template<typename F, typename Traits = raw_ptr_traits>
v8::Local<v8::FunctionTemplate> wrap_function_template(v8::Isolate* isolate, F&& func)
{
	using F_type = typename std::decay<F>::type;
	return v8::FunctionTemplate::New(isolate,
		&detail::forward_function<Traits, F_type>,
		detail::external_data::set(isolate, std::forward<F_type>(func)));
}

/// Wrap C++ function into new V8 function
/// Set nullptr or empty string for name
/// to make the function anonymous
template<typename F, typename Traits = raw_ptr_traits>
v8::Local<v8::Function> wrap_function(v8::Isolate* isolate, string_view name, F&& func)
{
	using F_type = typename std::decay<F>::type;
	v8::Local<v8::Function> fn = v8::Function::New(isolate->GetCurrentContext(),
		&detail::forward_function<Traits, F_type>,
		detail::external_data::set(isolate, std::forward<F_type>(func))).ToLocalChecked();
	if (!name.empty())
	{
		fn->SetName(to_v8(isolate, name));
	}
	return fn;
}

} // namespace v8pp

#endif // V8PP_FUNCTION_HPP_INCLUDED
