#ifndef V8PP_FORWARD_HPP_INCLUDED
#define V8PP_FORWARD_HPP_INCLUDED

#include <boost/mpl/front.hpp>
#include <boost/mpl/equal_to.hpp>

#include "v8pp/call_from_v8.hpp"
#include "v8pp/proto.hpp"
#include "v8pp/throw_ex.hpp"

namespace v8pp {

namespace detail {

template<typename P>
struct pass_direct_if : boost::mpl::and_<
	boost::mpl::equal_to< boost::mpl::size<typename P::arguments>, boost::mpl::int_<1> >,
	boost::is_same<v8::FunctionCallbackInfo<v8::Value> const&, typename boost::mpl::front<typename P::arguments>::type> >
{
};

template<typename P>
struct is_void_return : boost::is_same<void, typename P::return_type>::type
{
};

template<typename T>
struct is_function_pointer : boost::mpl::and_<
		boost::is_pointer<T>,
		boost::is_function<typename boost::remove_pointer<T>::type>
	>
{
};

template<typename P>
typename boost::enable_if<pass_direct_if<P>, typename P::return_type>::type
invoke(typename P::function_type ptr, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return (*ptr)(args);
}

template<typename P>
typename boost::disable_if<pass_direct_if<P>, typename P::return_type>::type
invoke(typename P::function_type ptr, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return call_from_v8<P>(ptr, args);
}

template<typename P, typename T>
typename boost::enable_if<pass_direct_if<P>, typename P::return_type>::type
invoke(T& obj, typename P::method_type ptr, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return (obj.*ptr)(args);
}

template<typename P, typename T>
typename boost::disable_if<pass_direct_if<P>, typename P::return_type>::type
invoke(T& obj, typename P::method_type ptr, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	return call_from_v8<P>(obj, ptr, args);
}

template<typename P>
typename boost::disable_if<is_void_return<P>, void>::type
forward_ret(typename P::function_type ptr, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(), invoke<P>(ptr, args)));
}

template<typename P>
typename boost::enable_if<is_void_return<P>, void>::type
forward_ret(typename P::function_type ptr, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	invoke<P>(ptr, args);
}

template<typename P, typename T>
typename boost::disable_if<is_void_return<P>, void>::type
forward_ret(T& obj, typename P::method_type ptr, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	args.GetReturnValue().Set(to_v8(args.GetIsolate(), invoke<P>(obj, ptr, args)));
}

template<typename P, typename T>
typename boost::enable_if<is_void_return<P>, void>::type
forward_ret(T& obj, typename P::method_type ptr, v8::FunctionCallbackInfo<v8::Value> const& args)
{
	invoke<P>(obj, ptr, args);
}

template<typename T>
union pointer_cast
{
private:
	void* ptr;
	T value;

public:
	explicit pointer_cast(void* ptr) : ptr(ptr)
	{
		BOOST_STATIC_ASSERT(sizeof(T) <= sizeof(void*));
	}
	explicit pointer_cast(T value) : value(value) {}

	operator void*() const { return ptr; }
	operator T() const { return value; }
};

template<typename T>
void delete_ext_data(v8::WeakCallbackData<v8::External, T> const& data)
{
	delete data.GetParameter();
}

template<typename T>
typename boost::enable_if_c<sizeof(T) <= sizeof(void*), v8::Local<v8::Value> >::type
set_external_data(v8::Isolate* isolate, T value)
{
	return v8::External::New(isolate, pointer_cast<T>(value));
}

template<typename T>
typename boost::disable_if_c<sizeof(T) <= sizeof(void*), v8::Local<v8::Value> >::type
set_external_data(v8::Isolate* isolate, T value)
{
	T* data = new T(value);

	v8::Local<v8::External> ext = v8::External::New(isolate, data);

	v8::Persistent<v8::External> pext(isolate, ext);
	pext.SetWeak(data, delete_ext_data<T>);

	return ext;
}

template<typename T>
typename boost::enable_if_c<sizeof(T) <= sizeof(void*), T>::type
get_external_data(v8::Handle<v8::Value> value)
{
	return pointer_cast<T>(value.As<v8::External>()->Value());
}

template<typename T>
typename boost::disable_if_c<sizeof(T) <= sizeof(void*), T>::type
get_external_data(v8::Handle<v8::Value> value)
{
	T* data = static_cast<T*>(value.As<v8::External>()->Value());
	return *data;
}

} // namespace detail

template<typename P>
void forward_function(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	v8::HandleScope scope(isolate);

	try
	{
		typedef typename P::function_type function_type;
		function_type ptr = detail::get_external_data<function_type>(args.Data());
		detail::forward_ret<P>(ptr, args);
	}
	catch (std::exception const& ex)
	{
		args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
	}
}

template<typename P>
void forward_mem_function(v8::FunctionCallbackInfo<v8::Value> const& args)
{
	v8::Isolate* isolate = args.GetIsolate();
	v8::HandleScope scope(isolate);

	try
	{
		typedef typename P::class_type class_type;
		typedef typename P::method_type method_type;

		class_type& obj = from_v8<class_type&>(args.GetIsolate(), args.Holder());
		method_type ptr = detail::get_external_data<method_type>(args.Data());
		detail::forward_ret<P>(obj, ptr, args);
	}
	catch (std::exception const& ex)
	{
		args.GetReturnValue().Set(throw_ex(isolate, ex.what()));
	}
}

} // namespace v8pp

#endif // V8PP_FORWARD_HPP_INCLUDED
