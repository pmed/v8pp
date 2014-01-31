#ifndef V8PP_PROPERTY_HPP_INCLUDED
#define V8PP_PROPERTY_HPP_INCLUDED

#include <boost/mpl/size.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>

#include "v8pp/proto.hpp"
#include "v8pp/to_v8.hpp"
#include "v8pp/from_v8.hpp"

namespace v8pp {

template<typename Get, typename Set>
struct property_;

namespace detail {

template<typename Get, typename Set>
struct r_property_impl
{
	typedef property_<Get, Set> Property;

	typedef typename boost::mpl::if_<is_function_pointer<Get>,
		function_ptr<Get>, mem_function_ptr<Get> >::type GetProto;

	static_assert(boost::mpl::size<typename GetProto::arguments>::value == 0,
		"property get function must have no arguments");

	static v8::Handle<v8::Value> get_impl(v8::Local<v8::String> name, v8::AccessorInfo const& info, boost::mpl::false_)
	{
		typedef typename GetProto::class_type class_type;
		class_type& obj = v8pp::from_v8<class_type&>(info.This());

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			return to_v8((obj.*prop.get_)());
		}
		catch (std::exception const& ex)
		{
			return throw_ex(ex.what());
		}
		return v8::Undefined();
	}

	static v8::Handle<v8::Value> get_impl(v8::Local<v8::String> name, v8::AccessorInfo const& info, boost::mpl::true_)
	{
		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);
		if (prop.get_)
		try
		{
			return to_v8((prop.get_)());
		}
		catch (std::exception const& ex)
		{
			return throw_ex(ex.what());
		}
		return v8::Undefined();
	}

	static v8::Handle<v8::Value> get(v8::Local<v8::String> name, v8::AccessorInfo const& info)
	{
		return get_impl(name, info, is_function_pointer<Get>());
	}

	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::AccessorInfo const& info)
	{
		assert(false && "never should be called");
	}
};

template<typename Get, typename Set>
struct rw_property_impl : r_property_impl<Get, Set>
{
	typedef property_<Get, Set> Property;

	typedef typename boost::mpl::if_<is_function_pointer<Set>,
		function_ptr<Set>, mem_function_ptr<Set> >::type SetProto;

	static_assert(boost::mpl::size<typename SetProto::arguments>::value == 1,
		"property set method must have single argument");
/*
	static_assert(boost::is_same<
		typename boost::remove_const<typename GetProto::class_type>::type,
		typename boost::remove_const<typename SetProto::class_type>::type>::value,
		"property get and set methods must be in the same class");
*/
	static void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::AccessorInfo const& info, boost::mpl::false_)
	{
		typedef typename SetProto::class_type class_type;
		class_type& obj = v8pp::from_v8<class_type&>(info.This());

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			typedef typename boost::mpl::at_c<typename SetProto::arguments, 0>::type value_type;
			(obj.*prop.set_)(v8pp::from_v8<value_type>(value));
		}
		catch (std::exception const& ex)
		{
			throw_ex(ex.what());
		}
	}

	static void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::AccessorInfo const& info, boost::mpl::true_)
	{
		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			typedef typename boost::mpl::at_c<typename SetProto::arguments, 0>::type value_type;
			(*prop.set_)(v8pp::from_v8<value_type>(value));
		}
		catch (std::exception const& ex)
		{
			throw_ex(ex.what());
		}
	}

	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::AccessorInfo const& info)
	{
		return set_impl(name, value, info, is_function_pointer<Set>());
	}
};

} // namespace detail

// Property of class T with get and set methods
template<typename Get, typename Set>
struct property_ : detail::rw_property_impl<Get, Set>
{
	Get get_;
	Set set_;

	enum { is_readonly = false };
};

// Rad-only property class specialization for get only method
template<typename Get>
struct property_<Get, Get> : detail::r_property_impl<Get, Get>
{
	Get get_;

	enum { is_readonly = true };
};

// Create read/write property from get and set member functions
template<typename Get, typename Set>
property_<Get, Set> property(Get get, Set set)
{
	property_<Get, Set> prop;
	prop.get_ = get;
	prop.set_ = set;
	return prop;
}

// Create read-only property from a get function
template<typename Get>
property_<Get, Get> property(Get get)
{
	property_<Get, Get> prop;
	prop.get_ = get;
	return prop;
}

} // namespace v8pp

#endif // V8PP_PROPERTY_HPP_INCLUDED
