#ifndef V8PP_CONVERT_HPP_INCLUDED
#define V8PP_CONVERT_HPP_INCLUDED
/*
#include <v8.h>

#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

namespace v8pp  {

namespace detail {

// Get pointer to native object
template<typename T>
T get_object_field(v8::Handle<v8::Value> value)
{
	while (value->IsObject())
	{
		v8::Handle<v8::Object> obj = value->ToObject();
		if (obj->InternalFieldCount() != 1)
		{
			// no internal field, it's not a wrapped C++ object
			break;
		}
		T native = reinterpret_cast<T>(obj->GetAlignedPointerFromInternalField(0));
		if (native)
		{
			return native;
		}
		value = obj->GetPrototype();
	}
	return nullptr;
}

// Native objects registry. Monostate
template<typename T>
class object_registry
{
public:
#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
	typedef boost::unordered_map<void*, v8::Persistent<v8::Value> > objects;

	// use additional set to distiguish instances of T in the global registry
	static boost::unordered_set<T*>& instances()
	{
		static boost::unordered_set<T*> instances_;
		return instances_;
	}
#else
	typedef boost::unordered_map<T*, v8::Persistent<v8::Value> > objects;
#endif

	static void add(T* object, v8::Persistent<v8::Value> value)
	{
#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
		instances().insert(object);
#endif
		items().insert(std::make_pair(object, value));
	}

	static void remove(T* object, void (*destroy)(T*))
	{
#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
		instances().erase(object);
#endif
		typename objects::iterator it = items().find(object);
		if (it != items().end())
		{
			it->second.Dispose();
			items().erase(it);
			if (destroy)
			{
				destroy(object);
			}
		}
	}

	static void remove_all(void (*destroy)(T*))
	{
#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
		while (!instances().empty())
		{
			remove(*instances().begin(), destroy);
		}
#else
		while (!items().empty())
		{
			remove(items().begin()->first, destroy);
		}
#endif
	}

	static v8::Handle<v8::Value> find(T const* native)
	{
		v8::Handle<v8::Value> result;
		typename objects::iterator it = items().find(const_cast<T*>(native));
		if ( it != items().end() )
		{
			result = it->second;
		}
		return result;
	}

private:
	object_registry();
	~object_registry();
	object_registry(object_registry const&);
	object_registry& operator=(object_registry const&);

private:
	static objects& items();
};

#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
extern V8PP_API object_registry<void>::objects global_registry_objects_;

template<typename T>
inline typename object_registry<T>::objects& object_registry<T>::items()
{
	return global_registry_objects_;
}
#else
template<typename T>
inline typename object_registry<T>::objects& object_registry<T>::items()
{
	static objects items_;
	return items_;
}
#endif

typedef boost::mpl::vector<v8::Value, v8::Object, v8::Function> v8_types;

template<typename T>
struct is_v8_type : boost::mpl::contains<v8_types, T>
{
};

} // namespace detail

// Generic convertor
template<typename T, typename Enable = void>
struct convert
{
	//static bool is_valid(v8::Handle<v8::Value> value);

	//static T from_v8(v8::Handle<v8::Value> value);

	//static v8::Handle<v8::Value> to_v8(T const& value);
};

template<typename T>
typename convert<T>::type from_v8(v8::Handle<v8::Value> value)
{
	return convert<T>::from_v8(value);
}

template<typename T>
typename convert<T>::type from_v8(v8::Handle<v8::Value> value, T const& default_value)
{
	return convert<T>::is_valid(value)? convert<T>::from_v8(value) : default_value;
}

template<typename T>
typename boost::disable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T value)
{
	return convert<T>::to_v8(value);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T const* value)
{
	return convert<T>::to_v8(value);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T const& value)
{
	return convert<T>::to_v8(value);
}

template<typename T>
v8::Handle<T> to_v8(v8::Handle<T> value)
{
	return value;
}

template<typename T>
v8::Local<T> to_v8(v8::Local<T> value)
{
	return value;
}

template<typename T>
v8::Persistent<T> to_v8(v8::Persistent<T> value)
{
	return value;
}
/*
template<typename T>
typename boost::disable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T value)
{
	return convert<T>::to_v8(value);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T* value)
{
	return convert<T const*>::to_v8(value);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T const* value)
{
	return convert<T const*>::to_v8(value);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T& value)
{
	return convert<T&>::to_v8(value);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T const& value)
{
	return convert<T const&>::to_v8(value);
}
* /
// converter specializations for V8 Handles
template<typename T>
struct convert< v8::Handle<T>,
	typename boost::enable_if<detail::is_v8_type<T> >::type >
{
	typedef v8::Handle<T> type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return v8::Handle<T>::Cast(value);
	}

	static v8::Handle<T> from_v8(v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<T> to_v8(v8::Handle<T> value)
	{
		return value;
	}
};

template<typename T>
struct convert< v8::Local<T> >
{
	typedef v8::Handle<T> type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return v8::Handle<T>::Cast(value);
	}

	static v8::Handle<T> from_v8(v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<T> to_v8(v8::Local<T> value)
	{
		return value;
	}
};

template<typename T>
struct convert< v8::Persistent<T> >
{
	typedef v8::Handle<T> type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return v8::Handle<T>::Cast(value);
	}

	static v8::Handle<T> from_v8(v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<T> to_v8(v8::Persistent<T> value)
	{
		return value;
	}
};

// converter specializations for primitive types
template<>
struct convert<bool>
{
	typedef bool type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsBoolean();
	}

	static bool from_v8(v8::Handle<v8::Value> value)
	{
		return value->ToBoolean()->Value();
	}

	static v8::Handle<v8::Value> to_v8(bool value)
	{
		return v8::Boolean::New(value);
	}
};

template<typename T>
struct convert< T, typename boost::enable_if< boost::is_integral<T> >::type >
{
	typedef T type;

	enum { bits = sizeof(T) * CHAR_BIT, is_signed = boost::is_signed<T>::value };

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsNumber();
	}

	static T from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsNumber())
		{
			throw std::invalid_argument("expected Number");
		}

		if (bits <= 32)
		{
			if (is_signed)
			{
				return static_cast<T>(value->Int32Value());
			}
			else
			{
				return static_cast<T>(value->Uint32Value());
			}
		}
		else
		{
			return static_cast<T>(value->IntegerValue());
		}
	}

	static v8::Handle<v8::Value> to_v8(T value)
	{
		if (bits <= 32)
		{
			if (is_signed)
			{
				return v8::Integer::New(value);
			}
			else
			{
				return v8::Integer::NewFromUnsigned(value);
			}
		}
		else
		{
			return v8::Number::New(value);
		}
	}
};

template<typename T>
struct convert< T, typename boost::enable_if< boost::is_floating_point<T> >::type >
{
	typedef T type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsNumber();
	}

	static T from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsNumber())
		{
			throw std::invalid_argument("expected Number");
		}

		return static_cast<T>(value->NumberValue());
	}

	static v8::Handle<v8::Value> to_v8(T value)
	{
		return v8::Number::New(value);
	}
};

// converter specializations for string types
template<>
struct convert<std::string>
{
	typedef std::string type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static std::string from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsString())
		{
			throw std::invalid_argument("expected String");
		}

		v8::String::Utf8Value str(value);
		return std::string(*str, str.length());
	}

	static v8::Handle<v8::Value> to_v8(std::string const& value)
	{
		return v8::String::New(value.data(), value.length());
	}
};

template<>
struct convert<char const*>
{
/*
	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static std::string from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsString())
		{
			throw std::invalid_argument("expected String");
		}
		v8::String::Utf8Value str(value);
		return std::string(*str, str.length());
	}
* /
	static v8::Handle<v8::Value> to_v8(char const* value)
	{
		return v8::String::New(value? value : "");
	}
};

template<>
struct convert<std::wstring, boost::enable_if_c<sizeof(wchar_t) == sizeof(uint16_t)> >
{
	typedef std::wstring type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static std::wstring from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsString())
		{
			throw std::invalid_argument("expected String");
		}

		v8::String::Value str(value);
		return std::wstring(reinterpret_cast<wchar_t const*>(*str), str.length());
	}

	static v8::Handle<v8::Value> to_v8(std::wstring const& value)
	{
		return v8::String::New(reinterpret_cast<uint16_t const*>(value.data()), value.length());
	}
};

template<>
struct convert<wchar_t const*, boost::enable_if_c<sizeof(wchar_t) == sizeof(uint16_t)> >
{
/*
	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static std::wstring from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsString())
		{
			throw std::invalid_argument("expected String");
		}
		v8::String::Value str(value);
		return std::wstring(reinterpret_cast<wchar_t const*>(*str), str.length());
	}
* /
	static v8::Handle<v8::Value> to_v8(wchar_t const* value)
	{
		return v8::String::New(reinterpret_cast<uint16_t const*>(value? value : L""));
	}
};

// converter Array <-> std::vector
template<typename T, typename Alloc>
struct convert< std::vector<T, Alloc> >
{
	typedef std::vector<T, Alloc> type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsArray();
	}

	static std::vector<T, Alloc> from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsArray())
		{
			throw std::invalid_argument("expected Array");
		}
		v8::HandleScope scope;

		v8::Handle<v8::Array> array = value.As<v8::Array>();

		std::vector<T, Alloc> result;
		result.reserve(array->Length());
		for (uint32_t i = 0, count = array->Length(); i < count; ++i)
		{
			result.push_back(v8pp::from_v8<T>(array->Get(i)));
		}
		return result;
	}

	static v8::Handle<v8::Value> to_v8(std::vector<T, Alloc> const& value)
	{
		v8::HandleScope scope;

		uint32_t const size = static_cast<uint32_t>(value.size());
		v8::Handle<v8::Array> result = v8::Array::New(size);
		for (uint32_t i = 0; i < size; ++i)
		{
			result->Set(i, v8pp::to_v8(value[i]));
		}
		return scope.Close(result);
	}
};

// specializations for pointers and reference to user classes
template<typename T>
struct convert< T* >
{
	typedef T* type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static T* from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsObject())
		{
			throw std::invalid_argument("expected Object");
		}
		return detail::get_object_field<T*>(value);
	}

	static v8::Handle<v8::Value> to_v8(T const* value)
	{
		return detail::object_registry<T>::find(value);
	}
};

template<typename T>
struct convert< T >
{
	typedef T& type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static T& from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsObject())
		{
			throw std::invalid_argument("expected Object");
		}
		if (T* obj = detail::get_object_field<T*>(value))
		{
			return *obj;
		}
		throw std::invalid_argument(std::string("expected C++ wrapped object ") + typeid(T).name());
	}

	static v8::Handle<v8::Value> to_v8(T const& value)
	{
		return detail::object_registry<T>::find(&value);
	}
};

template<typename T>
struct convert<T&> : convert<T> {};

template<typename T>
struct convert<T const&> : convert<T> {};

} // namespace v8pp
*/
#endif // V8PP_CONVERT_HPP_INCLUDED
