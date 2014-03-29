#ifndef V8PP_CONVERT_HPP_INCLUDED
#define V8PP_CONVERT_HPP_INCLUDED

#include <v8.h>

#include <string>
#include <vector>
#include <map>
#include <iterator>

#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

namespace v8pp  {

namespace detail {

// Get pointer to native object
template<typename T>
T* get_object_ptr(v8::Handle<v8::Value> value)
{
	while (value->IsObject())
	{
		v8::Handle<v8::Object> obj = value->ToObject();
		if (obj->InternalFieldCount() == 2)
		{
			void* ptr = obj->GetAlignedPointerFromInternalField(0);
			type_caster* caster = static_cast<type_caster*>(obj->GetAlignedPointerFromInternalField(1));
			if (caster && caster->can_cast())
			{
				caster->cast(ptr, typeid(T));
			}
			return static_cast<T*>(ptr);
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
		items().insert(std::make_pair(most_derived_ptr(object), value));
	}

	static void remove(T* object, void (*destroy)(T*))
	{
#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
		instances().erase(object);
#endif
		typename objects::iterator it = items().find(most_derived_ptr(object));
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
		typename objects::iterator it = items().find(most_derived_ptr(native));
		if (it != items().end())
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

	template<typename U>
	static typename boost::enable_if<boost::is_polymorphic<U>, U*>::type most_derived_ptr(U const* object)
	{
		return reinterpret_cast<U*>(dynamic_cast<void*>(const_cast<U*>(object)));
	}

	template<typename U>
	static typename boost::disable_if<boost::is_polymorphic<U>, U*>::type most_derived_ptr(U const* object)
	{
		return const_cast<U*>(object);
	}
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

template<typename Iterator>
struct is_random_access_iterator :
	boost::is_same<
		typename std::iterator_traits<Iterator>::iterator_category,
		std::random_access_iterator_tag
	>::type
{
};

// A string that converts to Char const * (useful for fusion::invoke)
template<typename Char>
struct convertible_string : std::basic_string<Char>
{
	convertible_string(Char const *str, size_t len) : std::basic_string<Char>(str, len) {}

	operator Char const*() const { return this->c_str(); }
};

} // namespace detail

// Generic convertor
template<typename T, typename Enable = void>
struct convert;
/*
{
	typedef T result_type;

	static bool is_valid(v8::Handle<v8::Value> value);

	static result_type from_v8(v8::Handle<v8::Value> value);

	static v8::Handle<v8::Value> to_v8(T const& value);
};
*/

// converter specializations for string types
template<typename Char, typename Traits, typename Alloc>
struct convert< std::basic_string<Char, Traits, Alloc>,
	typename boost::enable_if_c<sizeof(Char) <= sizeof(uint16_t)>::type >
{
	typedef std::basic_string<Char, Traits, Alloc> result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static result_type from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsString())
		{
			throw std::invalid_argument("expected String");
		}

		if (sizeof(Char) == 1)
		{
			v8::String::Utf8Value const str(value);
			return result_type(reinterpret_cast<Char const*>(*str), str.length());
		}
		else
		{
			v8::String::Value const str(value);
			return result_type(reinterpret_cast<Char const*>(*str), str.length());
		}
	}

	static v8::Handle<v8::Value> to_v8(result_type const& value)
	{
		if (sizeof(Char) == 1)
		{
			return v8::String::New(reinterpret_cast<char const*>(value.data()), static_cast<int>(value.length()));
		}
		else
		{
			return v8::String::New(reinterpret_cast<uint16_t const*>(value.data()), static_cast<int>(value.length()));
		}
	}
};

template<typename Char>
struct convert<Char const*,
	typename boost::enable_if_c<sizeof(Char) <= sizeof(uint16_t)>::type >
{
	typedef detail::convertible_string<Char> result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static result_type from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsString())
		{
			throw std::invalid_argument("expected String");
		}

		if (sizeof(Char) == 1)
		{
			v8::String::Utf8Value const str(value);
			return result_type(reinterpret_cast<Char const*>(*str), str.length());
		}
		else
		{
			v8::String::Value const str(value);
			return result_type(reinterpret_cast<Char const*>(*str), str.length());
		}
	}

	static v8::Handle<v8::Value> to_v8(Char const* value)
	{
		if (sizeof(Char) == 1)
		{
			return v8::String::New(reinterpret_cast<char const*>(value));
		}
		else
		{
			return v8::String::New(reinterpret_cast<uint16_t const*>(value));
		}
	}
};

/*
template<>
struct convert<std::wstring, boost::enable_if_c<sizeof(wchar_t) == sizeof(uint16_t)>::type >
{
	typedef std::wstring result_type;

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
struct convert<wchar_t const*, boost::enable_if_c<sizeof(wchar_t) == sizeof(uint16_t)>::type >
{
	typedef detail::convertible_string<wchar_t> result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static result_type from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsString())
		{
			throw std::invalid_argument("expected String");
		}
		v8::String::Value str(value);
		return result_type(reinterpret_cast<wchar_t const*>(*str), str.length());
	}

	static v8::Handle<v8::Value> to_v8(wchar_t const* value)
	{
		return v8::String::New(reinterpret_cast<uint16_t const*>(value? value : L""));
	}
};
*/

// converter specializations for primitive types
template<>
struct convert<bool>
{
	typedef bool result_type;

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
	typedef T result_type;

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
struct convert< T, typename boost::enable_if< boost::is_enum<T> >::type >
{
	typedef T result_type;

#if __cplusplus < 201103L
	typedef int underlying_type;
#else
	typedef typename std::underlying_type<T>::type underlying_type;
#endif

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsNumber();
	}

	static T from_v8(v8::Handle<v8::Value> value)
	{
		return static_cast<T>(convert<underlying_type>::from_v8(value));
	}

	static v8::Handle<v8::Value> to_v8(T value)
	{
		return convert<underlying_type>::to_v8(value);
	}
};

template<typename T>
struct convert< T, typename boost::enable_if< boost::is_floating_point<T> >::type >
{
	typedef T result_type;

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

// convert Array <-> std::vector
template<typename T, typename Alloc>
struct convert< std::vector<T, Alloc> >
{
	typedef std::vector<T, Alloc> result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsArray();
	}

	static result_type from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsArray())
		{
			throw std::invalid_argument("expected Array");
		}
		v8::HandleScope scope;

		v8::Handle<v8::Array> array = value.As<v8::Array>();

		result_type result;
		result.reserve(array->Length());
		for (uint32_t i = 0, count = array->Length(); i < count; ++i)
		{
			result.push_back(convert<T>::from_v8(array->Get(i)));
		}
		return result;
	}

	static v8::Handle<v8::Value> to_v8(result_type const& value)
	{
		v8::HandleScope scope;

		uint32_t const size = static_cast<uint32_t>(value.size());
		v8::Handle<v8::Array> result = v8::Array::New(size);
		for (uint32_t i = 0; i < size; ++i)
		{
			result->Set(i, convert<T>::to_v8(value[i]));
		}
		return scope.Close(result);
	}
};

// convert Object <-> std::map
template<typename Key, typename Value, typename Less, typename Alloc>
struct convert< std::map<Key, Value, Less, Alloc> >
{
	typedef std::map<Key, Value, Less, Alloc> result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static result_type from_v8(v8::Handle<v8::Value> value)
	{
		if (!value->IsObject())
		{
			throw std::invalid_argument("expected Object");
		}
		v8::HandleScope scope;

		v8::Handle<v8::Object> object = value.As<v8::Object>();
		v8::Handle<v8::Array> prop_names = object->GetPropertyNames();

		result_type result;
		for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
		{
			v8::Handle<v8::Value> key = prop_names->Get(i);
			v8::Handle<v8::Value> val = object->Get(key);
			result.insert(std::make_pair(convert<Key>::from_v8(key), convert<Value>::from_v8(val)));
		}
		return result;
	}

	static v8::Handle<v8::Value> to_v8(result_type const& value)
	{
		v8::HandleScope scope;

		v8::Handle<v8::Object> result = v8::Object::New();
		for (typename result_type::const_iterator it = value.begin(), end = value.end(); it != end; ++it)
		{
			result->Set(convert<Key>::to_v8(it->first), convert<Value>::to_v8(it->second));
		}
		return scope.Close(result);
	}
};

// converter specializations for V8 Handles
template<typename T>
struct convert< v8::Handle<T> >
{
	typedef v8::Handle<T> result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<v8::Value> to_v8(v8::Handle<T> value)
	{
		return value;
	}
};

template<typename T>
struct convert< v8::Local<T> >
{
	typedef v8::Handle<T> result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<v8::Value> to_v8(v8::Local<T> value)
	{
		return value;
	}
};

template<typename T>
struct convert< v8::Persistent<T> >
{
	typedef v8::Handle<T> result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<v8::Value> to_v8(v8::Persistent<T> value)
	{
		return value;
	}
};

// convert specialization for wrapped user classes
template<typename T>
struct is_wrapped_class	: boost::is_class<T> {};

template<typename T>
struct is_wrapped_class< v8::Handle<T> > : boost::false_type {};

template<typename T>
struct is_wrapped_class< v8::Local<T> > : boost::false_type {};

template<typename T>
struct is_wrapped_class< v8::Persistent<T> > : boost::false_type {};

template<typename Char, typename Traits, typename Alloc>
struct is_wrapped_class< std::basic_string<Char, Traits, Alloc> > : boost::false_type {};

template<typename T, typename Alloc>
struct is_wrapped_class< std::vector<T, Alloc> > : boost::false_type {};

template<typename Key, typename Value, typename Less, typename Alloc>
struct is_wrapped_class< std::map<Key, Value, Less, Alloc> > : boost::false_type {};

template<typename T>
struct convert< T, typename boost::enable_if< is_wrapped_class<T> >::type >
{
	typedef T& result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static T& from_v8(v8::Handle<v8::Value> value)
	{
		if (T* obj_ptr = detail::get_object_ptr<T>(value))
		{
			return *obj_ptr;
		}
		throw std::runtime_error(std::string("expected C++ wrapped object ") + typeid(T).name());
	}

	static v8::Handle<v8::Value> to_v8(T const& value)
	{
		return detail::object_registry<T>::find(&value);
	}
};

template<typename T>
struct convert< T& > : convert<T> {};

template<typename T>
struct convert< T const& > : convert<T> {};

template<typename T>
struct convert< T*, typename boost::enable_if< is_wrapped_class<T> >::type >
{
	typedef T* result_type;

	static bool is_valid(v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static T* from_v8(v8::Handle<v8::Value> value)
	{
		return detail::get_object_ptr<T>(value);
	}

	static v8::Handle<v8::Value> to_v8(T const* value)
	{
		return detail::object_registry<typename boost::remove_cv<T>::type>::find(value);
	}
};

template<typename T>
inline typename convert<T>::result_type from_v8(v8::Handle<v8::Value> value)
{
	return convert<T>::from_v8(value);
}

template<typename T, typename U>
inline typename convert<T>::result_type from_v8(v8::Handle<v8::Value> value, U const& default_value)
{
	return convert<T>::is_valid(value)? convert<T>::from_v8(value) : default_value;
}

template<typename T>
inline typename boost::disable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T value)
{
	return convert<T>::to_v8(value);
}

template<typename T>
inline typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(T const& value)
{
	return convert<T>::to_v8(value);
}

template<typename Iterator>
inline typename boost::enable_if<detail::is_random_access_iterator<Iterator>, v8::Handle<v8::Value> >::type
to_v8(Iterator begin, Iterator end)
{
	v8::HandleScope scope;

	v8::Handle<v8::Array> result = v8::Array::New(end - begin);
	for (uint32_t idx = 0; begin != end; ++begin, ++idx)
	{
		result->Set(idx, to_v8(*begin));
	}
	return scope.Close(result);
}

template<typename Iterator>
inline typename boost::disable_if<detail::is_random_access_iterator<Iterator>, v8::Handle<v8::Value> >::type
to_v8(Iterator begin, Iterator end)
{
	v8::HandleScope scope;

	v8::Handle<v8::Array> result = v8::Array::New();
	for (uint32_t idx = 0; begin != end; ++begin, ++idx)
	{
		result->Set(idx, to_v8(*begin));
	}
	return scope.Close(result);
}

} // namespace v8pp

#endif // V8PP_CONVERT_HPP_INCLUDED
