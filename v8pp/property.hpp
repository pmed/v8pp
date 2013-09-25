#ifndef V8PP_PROPERTY_HPP_INCLUDED
#define V8PP_PROPERTY_HPP_INCLUDED

#include <boost/mpl/size.hpp>
#include <boost/type_traits/is_same.hpp>

#include "proto.hpp"
#include "to_v8.hpp"
#include "from_v8.hpp"

namespace v8pp {

// Property of class T with get and set methods
template<typename Get, typename Set>
struct property_t
{
	Get get_;
	Set set_;

	typedef typename detail::mem_function_ptr<Get> GetProto;
	typedef typename detail::mem_function_ptr<Set> SetProto;

	static_assert(boost::is_same<typename GetProto::class_type, typename SetProto::class_type>::value,
		"property get and set methods must be in the same class");

	static_assert(boost::mpl::size<typename GetProto::arguments>::value == 0,
		"property get method must have no arguments");

	static_assert(boost::mpl::size<typename SetProto::arguments>::value == 1,
		"property set method must have single argument");

	enum { is_readonly = false };

	static v8::Handle<v8::Value> get(v8::Local<v8::String> name, v8::AccessorInfo const& info)
	{
		GetProto::class_type const& obj = v8pp::from_v8<GetProto::class_type const&>(info.This());
		property_t const prop = detail::get_external_data<property_t>(info.Data());
		assert(prop.get_);
		try
		{
			return to_v8((obj.*prop.get_)());
		}
		catch (std::exception const& ex)
		{
			return throw_ex(ex.what());
		}
	}

	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::AccessorInfo const& info)
	{
		SetProto::class_type& obj = v8pp::from_v8<SetProto::class_type&>(info.This());
		property_t const prop = detail::get_external_data<property_t>(info.Data());
		assert(prop.set_);
		try
		{
			typedef typename boost::mpl::at_c<SetProto::arguments, 0>::type value_type;
			(obj.*prop.set_)(v8pp::from_v8<value_type>(value));
		}
		catch (std::exception const&)
		{
			//?? return throw_ex(ex.what());
		}
	}
};

// Rad-only property class specialization for get only method
template<typename Get>
struct property_t<Get, Get>
{
	Get get_;

	typedef typename detail::mem_function_ptr<Get> GetProto;

	static_assert(boost::mpl::size<typename GetProto::arguments>::value == 0,
		"property get method must have no arguments");

	enum { is_readonly = true };

	static v8::Handle<v8::Value> get(v8::Local<v8::String> name, v8::AccessorInfo const& info)
	{
		GetProto::class_type const& obj = v8pp::from_v8<GetProto::class_type const&>(info.This());
		property_t const prop = detail::get_external_data<property_t>(info.Data());
		assert(prop.get_);
		try
		{
			return to_v8((obj.*prop.get_)());
		}
		catch (std::exception const& ex)
		{
			return throw_ex(ex.what());
		}
	}

	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::AccessorInfo const& info)
	{
		assert(false && "never should be called");
	}
};

// Create read/write property from get and set member functions
template<typename Get, typename Set>
property_t<Get, Set> property(Get get, Set set)
{
	property_t<Get, Set> prop;
	prop.get_ = get;
	prop.set_ = set;
	return prop;
}

// Create read-only property from a get function
template<typename Get>
property_t<Get, Get> property(Get get)
{
	property_t<Get, Get> prop;
	prop.get_ = get;
	return prop;
}

} // namespace v8pp

#endif // V8PP_PROPERTY_HPP_INCLUDED
