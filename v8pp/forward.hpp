#ifndef V8PP_FORWARD_HPP_INCLUDED
#define V8PP_FORWARD_HPP_INCLUDED

#include "v8pp/from_v8.hpp"
#include "v8pp/call_from_v8.hpp"
#include "v8pp/proto.hpp"
#include "v8pp/throw_ex.hpp"
#include "v8pp/to_v8.hpp"

namespace v8pp {

namespace detail {

template<typename P>
struct pass_direct_if : boost::is_same<v8::Arguments const&,
	typename boost::mpl::front<typename P::arguments>::type>
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
invoke(typename P::function_type ptr, v8::Arguments const& args)
{
	return (*ptr)(args);
}

template<typename P>
typename boost::disable_if<pass_direct_if<P>, typename P::return_type>::type
invoke(typename P::function_type ptr, v8::Arguments const& args)
{
	return call_from_v8<P>(ptr, args);
}

template<typename P, typename T>
typename boost::enable_if<pass_direct_if<P>, typename P::return_type>::type
invoke(T* obj, typename P::method_type ptr, v8::Arguments const& args)
{
	return (obj->*ptr)(args);
}

template<typename P, typename T>
typename boost::disable_if<pass_direct_if<P>, typename P::return_type>::type
invoke(T* obj, typename P::method_type ptr, v8::Arguments const& args)
{
	return call_from_v8<P>(*obj, ptr, args);
}

template<typename P>
typename boost::disable_if<is_void_return<P>, v8::Handle<v8::Value> >::type
forward_ret(typename P::function_type ptr, const v8::Arguments& args)
{
	return to_v8(invoke<P>(ptr, args));
}

template<typename P>
typename boost::enable_if<is_void_return<P>, v8::Handle<v8::Value> >::type
forward_ret(typename P::function_type ptr, const v8::Arguments& args)
{
	invoke<P>(ptr, args);
	return v8::Undefined();
}

template<typename P, typename T>
typename boost::disable_if<is_void_return<P>, v8::Handle<v8::Value> >::type
forward_ret(T *obj, typename P::method_type ptr, v8::Arguments const& args)
{
	return to_v8(invoke<P>(obj, ptr, args));
}

template<typename P, typename T>
typename boost::enable_if<is_void_return<P>, v8::Handle<v8::Value> >::type
forward_ret(T *obj, typename P::method_type ptr, v8::Arguments const& args)
{
	invoke<P>(obj, ptr, args);
	return v8::Undefined();
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
typename boost::enable_if_c<sizeof(T) <= sizeof(void*), v8::Handle<v8::Value> >::type
set_external_data(T value)
{
	return v8::External::New(pointer_cast<T>(value));
}

template<typename T>
typename boost::disable_if_c<sizeof(T) <= sizeof(void*), v8::Handle<v8::Value> >::type
set_external_data(T value)
{
	return v8::External::New(new T(value));
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
	return *reinterpret_cast<T*>(value.As<v8::External>()->Value());
}

} // namespace detail

template<typename P>
v8::Handle<v8::Value> forward_function(const v8::Arguments& args)
{
	v8::HandleScope scope;

	try
	{
		typedef typename P::function_type function_type;
		function_type ptr = detail::get_external_data<function_type>(args.Data());
		return scope.Close(detail::forward_ret<P>(ptr, args));
	}
	catch (std::exception const& ex)
	{
		return scope.Close(throw_ex(ex.what()));
	}
}

template<typename P>
v8::Handle<v8::Value> forward_mem_function(v8::Arguments const& args)
{
	v8::HandleScope scope;

	try
	{
		typedef typename P::class_type class_type;
		typedef typename P::method_type method_type;

		class_type* obj = detail::get_object_field<class_type*>(args.Holder());
		method_type ptr = detail::get_external_data<method_type>(args.Data());
		return scope.Close(detail::forward_ret<P>(obj, ptr, args));
	}
	catch (std::exception const& ex)
	{
		return scope.Close(throw_ex(ex.what()));
	}
}

} // namespace v8pp

#endif // V8PP_FORWARD_HPP_INCLUDED
