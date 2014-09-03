#ifndef V8PP_PROPERTY_HPP_INCLUDED
#define V8PP_PROPERTY_HPP_INCLUDED

#include <boost/mpl/front.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/if.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>

#include "v8pp/proto.hpp"
#include "v8pp/convert.hpp"

namespace v8pp {

template<typename Get, typename Set>
struct property_;

namespace detail {

struct getter_tag {};
struct direct_getter_tag {};
struct isolate_getter_tag {};

struct setter_tag {};
struct direct_setter_tag {};
struct isolate_setter_tag {};

template<typename P>
struct is_getter : boost::mpl::and_<
	boost::mpl::equal_to< boost::mpl::size<typename P::arguments>, boost::mpl::int_<0> >,
	boost::mpl::not_<boost::is_same<void, typename P::return_type> > >
{
};

template<typename P>
struct is_direct_getter : boost::mpl::and_<
	boost::mpl::equal_to< boost::mpl::size<typename P::arguments>, boost::mpl::int_<1> >,
	boost::is_same<v8::PropertyCallbackInfo<v8::Value> const&, typename boost::mpl::front<typename P::arguments>::type>,
	boost::is_same<void, typename P::return_type> >
{
};

template<typename P>
struct is_isolate_getter : boost::mpl::and_<
	boost::mpl::equal_to< boost::mpl::size<typename P::arguments>, boost::mpl::int_<1> >,
	boost::is_same<v8::Isolate*, typename boost::mpl::front<typename P::arguments>::type>,
	boost::mpl::not_<boost::is_same<void, typename P::return_type> > >
{
};

template<typename P>
struct is_setter : boost::mpl::and_<
	boost::mpl::equal_to< boost::mpl::size<typename P::arguments>, boost::mpl::int_<1> >,
	boost::is_same<void, typename P::return_type> >
{
};

template<typename P>
struct is_direct_setter : boost::mpl::and_<
	boost::mpl::equal_to< boost::mpl::size<typename P::arguments>, boost::mpl::int_<1> >,
	boost::is_same<v8::PropertyCallbackInfo<void> const&, typename boost::mpl::front<typename P::arguments>::type>,
	boost::is_same<void, typename P::return_type> >
{
};

template<typename P>
struct is_isolate_setter : boost::mpl::and_<
	boost::mpl::equal_to< boost::mpl::size<typename P::arguments>, boost::mpl::int_<2> >,
	boost::is_same<v8::Isolate*, typename boost::mpl::front<typename P::arguments>::type>,
	boost::is_same<void, typename P::return_type> >
{
};

template<typename P>
struct select_getter_tag : boost::mpl::if_<is_getter<P>, getter_tag,
	typename boost::mpl::if_<is_direct_getter<P>, direct_getter_tag, isolate_getter_tag>::type >::type
{
};

template<typename P>
struct select_setter_tag : boost::mpl::if_<is_setter<P>, setter_tag,
	typename boost::mpl::if_<is_direct_setter<P>, direct_setter_tag, isolate_setter_tag>::type >::type
{
};

template<typename Get, typename Set>
struct r_property_impl
{
	typedef property_<Get, Set> Property;

	typedef typename boost::mpl::if_<is_function_pointer<Get>,
		function_ptr<Get>, mem_function_ptr<Get> >::type GetProto;

	static_assert(is_getter<GetProto>::value
		|| is_direct_getter<GetProto>::value
		|| is_isolate_getter<GetProto>::value,
		"property get function must be either T() or \
			void (v8::PropertyCallbackInfo<v8::Value> const&) or T(v8::Isolate*)");

	template<typename T>
	static void get_impl(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		boost::mpl::false_ is_fun_ptr, getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		T& obj = v8pp::from_v8<T&>(isolate, info.This());

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			info.GetReturnValue().Set(to_v8(isolate, (obj.*prop.get_)()));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void get_impl(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		boost::mpl::false_ is_fun_ptr, direct_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		T& obj = v8pp::from_v8<T&>(isolate, info.This());

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			(obj.*prop.get_)(info);
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void get_impl(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		boost::mpl::false_ is_fun_ptr, isolate_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		T& obj = v8pp::from_v8<T&>(isolate, info.This());

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			info.GetReturnValue().Set(to_v8(isolate, (obj.*prop.get_)(isolate)));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void get_impl(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		boost::mpl::true_ is_fun_ptr, getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			info.GetReturnValue().Set(to_v8(isolate, (prop.get_)()));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void get_impl(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		boost::mpl::true_ is_fun_ptr, direct_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			(prop.get_)(info);
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void get_impl(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info,
		boost::mpl::true_ is_fun_ptr, isolate_getter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.get_);

		if (prop.get_)
		try
		{
			info.GetReturnValue().Set(to_v8(isolate, (prop.get_)(isolate)));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void get(v8::Local<v8::String> name, v8::PropertyCallbackInfo<v8::Value> const& info)
	{
		get_impl<T>(name, info, is_function_pointer<Get>(), select_getter_tag<GetProto>());
	}

	template<typename T>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		assert(false && "never should be called");
	}
};

template<typename Get, typename Set>
struct rw_property_impl : r_property_impl<Get, Set>
{
	typedef property_<Get, Set> Property;

	typedef typename r_property_impl<Get, Set>::GetProto GetProto;

	typedef typename boost::mpl::if_<is_function_pointer<Set>,
		function_ptr<Set>, mem_function_ptr<Set> >::type SetProto;

	static_assert(is_setter<SetProto>::value
		|| is_direct_setter<SetProto>::value
		|| is_isolate_setter<SetProto>::value,
		"property set function must be either (T) or \
			void (v8::PropertyCallbackInfo<v8::Value> const&) or void (v8::Isolate*, T)");

	static_assert(boost::is_same<
		typename boost::remove_const<typename GetProto::class_type>::type,
		typename boost::remove_const<typename SetProto::class_type>::type>::value,
		"property get and set methods must be in the same class");

	template<typename T>
	static void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		boost::mpl::false_ is_fun_ptr, setter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		T& obj = v8pp::from_v8<T&>(isolate, info.This());

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			typedef typename boost::mpl::at_c<typename SetProto::arguments, 0>::type value_type;
			(obj.*prop.set_)(v8pp::from_v8<value_type>(isolate, value));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		boost::mpl::false_ is_fun_ptr, direct_setter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		T& obj = v8pp::from_v8<T&>(isolate, info.This());

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			(obj.*prop.set_)(info);
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		boost::mpl::false_ is_fun_ptr, isolate_setter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		T& obj = v8pp::from_v8<T&>(isolate, info.This());

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			typedef typename boost::mpl::at_c<typename SetProto::arguments, 0>::type value_type;
			(obj.*prop.set_)(isolate, v8pp::from_v8<value_type>(isolate, value));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		boost::mpl::true_ is_fun_ptr, setter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			typedef typename boost::mpl::at_c<typename SetProto::arguments, 0>::type value_type;
			(*prop.set_)(v8pp::from_v8<value_type>(isolate, value));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		boost::mpl::true_ is_fun_ptr, direct_setter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			(*prop.set_)(info);
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void set_impl(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info,
		boost::mpl::true_ is_fun_ptr, isolate_setter_tag)
	{
		v8::Isolate* isolate = info.GetIsolate();

		Property const prop = detail::get_external_data<Property>(info.Data());
		assert(prop.set_);

		if (prop.set_)
		try
		{
			typedef typename boost::mpl::at_c<typename SetProto::arguments, 1>::type value_type;
			(*prop.set_)(isolate, v8pp::from_v8<value_type>(isolate, value));
		}
		catch (std::exception const& ex)
		{
			info.GetReturnValue().Set(throw_ex(isolate, ex.what()));
		}
	}

	template<typename T>
	static void set(v8::Local<v8::String> name, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info)
	{
		set_impl<T>(name, value, info, is_function_pointer<Set>(), select_setter_tag<SetProto>());
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
