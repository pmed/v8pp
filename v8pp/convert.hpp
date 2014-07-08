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

template<typename T>
class class_;

// Generic convertor
template<typename T, typename Enable = void>
struct convert;
/*
{
	typedef T result_type;

	static bool is_valid(v8::Isolate* isolate, v8::Handle<v8::Value> value);

	static result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value);

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, T const& value);
};
*/

// converter specializations for string types
template<typename Char, typename Traits, typename Alloc>
struct convert< std::basic_string<Char, Traits, Alloc>,
	typename boost::enable_if_c<sizeof(Char) <= sizeof(uint16_t)>::type >
{
	typedef std::basic_string<Char, Traits, Alloc> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static result_type from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
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

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, result_type const& value)
	{
		if (sizeof(Char) == 1)
		{
			return v8::String::NewFromUtf8(isolate, reinterpret_cast<char const*>(value.data()), v8::String::kNormalString, value.length());
		}
		else
		{
			return v8::String::NewFromTwoByte(isolate, reinterpret_cast<uint16_t const*>(value.data()), v8::String::kNormalString, value.length());
		}
	}
};

template<typename Char>
struct convert<Char const*,
	typename boost::enable_if_c<sizeof(Char) <= sizeof(uint16_t)>::type >
{
	typedef detail::convertible_string<Char> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsString();
	}

	static result_type from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
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

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, Char const* value)
	{
		if (sizeof(Char) == 1)
		{
			return v8::String::NewFromUtf8(isolate, reinterpret_cast<char const*>(value));
		}
		else
		{
			return v8::String::NewFromTwoByte(isolate, reinterpret_cast<uint16_t const*>(value));
		}
	}
};

// converter specializations for primitive types
template<>
struct convert<bool>
{
	typedef bool result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsBoolean();
	}

	static bool from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->ToBoolean()->Value();
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, bool value)
	{
		return v8::Boolean::New(isolate, value);
	}
};

template<typename T>
struct convert< T, typename boost::enable_if< boost::is_integral<T> >::type >
{
	typedef T result_type;

	enum { bits = sizeof(T) * CHAR_BIT, is_signed = boost::is_signed<T>::value };

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsNumber();
	}

	static T from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
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

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, T value)
	{
		if (bits <= 32)
		{
			if (is_signed)
			{
				return v8::Integer::New(isolate, value);
			}
			else
			{
				return v8::Integer::NewFromUnsigned(isolate, value);
			}
		}
		else
		{
			return v8::Number::New(isolate, value);
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

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsNumber();
	}

	static T from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		return static_cast<T>(convert<underlying_type>::from_v8(isolate, value));
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, T value)
	{
		return convert<underlying_type>::to_v8(isolate, value);
	}
};

template<typename T>
struct convert< T, typename boost::enable_if< boost::is_floating_point<T> >::type >
{
	typedef T result_type;

	static bool is_valid(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		return value->IsNumber();
	}

	static T from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		if (!value->IsNumber())
		{
			throw std::invalid_argument("expected Number");
		}

		return static_cast<T>(value->NumberValue());
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, T value)
	{
		return v8::Number::New(isolate, value);
	}
};

// convert Array <-> std::vector
template<typename T, typename Alloc>
struct convert< std::vector<T, Alloc> >
{
	typedef std::vector<T, Alloc> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsArray();
	}

	static result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!value->IsArray())
		{
			throw std::invalid_argument("expected Array");
		}

		v8::Handle<v8::Array> array = value.As<v8::Array>();

		result_type result;
		result.reserve(array->Length());
		for (uint32_t i = 0, count = array->Length(); i < count; ++i)
		{
			result.push_back(convert<T>::from_v8(isolate, array->Get(i)));
		}
		return result;
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, result_type const& value)
	{
		uint32_t const size = static_cast<uint32_t>(value.size());
		v8::Local<v8::Array> result = v8::Array::New(isolate, size);
		for (uint32_t i = 0; i < size; ++i)
		{
			result->Set(i, convert<T>::to_v8(isolate, value[i]));
		}
		return result;
	}
};

// convert Object <-> std::map
template<typename Key, typename Value, typename Less, typename Alloc>
struct convert< std::map<Key, Value, Less, Alloc> >
{
	typedef std::map<Key, Value, Less, Alloc> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (!value->IsObject())
		{
			throw std::invalid_argument("expected Object");
		}

		v8::Handle<v8::Object> object = value.As<v8::Object>();
		v8::Handle<v8::Array> prop_names = object->GetPropertyNames();

		result_type result;
		for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
		{
			v8::Handle<v8::Value> key = prop_names->Get(i);
			v8::Handle<v8::Value> val = object->Get(key);
			result.insert(std::make_pair(convert<Key>::from_v8(key), convert<Value>::from_v8(isolate, val)));
		}
		return result;
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, result_type const& value)
	{
		v8::Handle<v8::Object> result = v8::Object::New(isolate);
		for (typename result_type::const_iterator it = value.begin(), end = value.end(); it != end; ++it)
		{
			result->Set(convert<Key>::to_v8(isolate, it->first), convert<Value>::to_v8(isolate, it->second));
		}
		return result;
	}
};

// converter specializations for V8 Handles
template<typename T>
struct convert< v8::Handle<T> >
{
	typedef v8::Handle<T> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate*, v8::Handle<T> value)
	{
		return value;
	}
};

template<typename T>
struct convert< v8::Local<T> >
{
	typedef v8::Handle<T> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate*, v8::Local<T> value)
	{
		return value;
	}
};

template<typename T>
struct convert< v8::Persistent<T> >
{
	typedef v8::Handle<T> result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Handle<T> from_v8(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate*, v8::Persistent<T> value)
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
struct convert< T*, typename boost::enable_if< is_wrapped_class<T> >::type >
{
	typedef T* result_type;
	typedef typename boost::remove_cv<T>::type class_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static T* from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		return class_<class_type>::unwrap_object(isolate, value);
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, T const* value)
	{
		return class_<class_type>::find_object(isolate, value);
	}
};

template<typename T>
struct convert< T, typename boost::enable_if< is_wrapped_class<T> >::type >
{
	typedef T& result_type;

	static bool is_valid(v8::Isolate*, v8::Handle<v8::Value> value)
	{
		return value->IsObject();
	}

	static T& from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
	{
		if (T* object = convert<T*>::from_v8(isolate, value))
		{
			return *object;
		}
		throw std::runtime_error(std::string("expected C++ wrapped object ") + typeid(T).name());
	}

	static v8::Handle<v8::Value> to_v8(v8::Isolate* isolate, T const& value)
	{
		return convert<T*>::to_v8(isolate, &value);
	}
};

template<typename T>
struct convert< T& > : convert<T> {};

template<typename T>
struct convert< T const& > : convert<T> {};

template<typename T>
inline typename convert<T>::result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
	return convert<T>::from_v8(isolate, value);
}

template<typename T, typename U>
inline typename convert<T>::result_type from_v8(v8::Isolate* isolate, v8::Handle<v8::Value> value, U const& default_value)
{
	return convert<T>::is_valid(isolate, value)? convert<T>::from_v8(isolate, value) : default_value;
}

template<typename T>
inline typename boost::disable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(v8::Isolate* isolate, T value)
{
	return convert<T>::to_v8(isolate, value);
}

template<typename T>
inline typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type
to_v8(v8::Isolate* isolate, T const& value)
{
	return convert<T>::to_v8(isolate, value);
}

template<typename Iterator>
inline typename boost::enable_if<detail::is_random_access_iterator<Iterator>, v8::Handle<v8::Value> >::type
to_v8(v8::Isolate* isolate, Iterator begin, Iterator end)
{
	v8::Handle<v8::Array> result = v8::Array::New(isolate, end - begin);
	for (uint32_t idx = 0; begin != end; ++begin, ++idx)
	{
		result->Set(idx, to_v8(isolate, *begin));
	}
	return result;
}

template<typename Iterator>
inline typename boost::disable_if<detail::is_random_access_iterator<Iterator>, v8::Handle<v8::Value> >::type
to_v8(v8::Isolate* isolate, Iterator begin, Iterator end)
{
	v8::Handle<v8::Array> result = v8::Array::New(isolate);
	for (uint32_t idx = 0; begin != end; ++begin, ++idx)
	{
		result->Set(idx, to_v8(isolate, *begin));
	}
	return result;
}

inline v8::Handle<v8::String> to_v8(v8::Isolate* isolate, char const* str, int len = -1)
{
	return v8::String::NewFromUtf8(isolate, str, v8::String::kNormalString, len);
}

#if OS(WINDOWS)
inline v8::Handle<v8::String> to_v8(v8::Isolate* isolate, wchar_t const* str, int len = -1)
{
	return v8::String::NewFromTwoByte(isolate, reinterpret_cast<uint16_t const*>(str), v8::String::kNormalString, len);
}
#endif

template<typename T>
v8::Local<T> to_local(v8::Isolate* isolate, v8::PersistentBase<T> const& handle)
{
	if (handle.IsWeak())
	{
		return v8::Local<T>::New(isolate, handle);
	}
	else
	{
		return *reinterpret_cast<v8::Local<T>*>(const_cast<v8::PersistentBase<T>*>(&handle));
	}
}

} // namespace v8pp

#endif // V8PP_CONVERT_HPP_INCLUDED
