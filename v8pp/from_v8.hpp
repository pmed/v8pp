#ifndef V8PP_FROM_V8_HPP_INCLUDED
#define V8PP_FROM_V8_HPP_INCLUDED

#include <string>
#include <stdexcept>
#include <vector>

#include <v8.h>

namespace v8pp {

namespace detail {

// Get pointer to native object
template<typename T>
T get_object_field(v8::Handle<v8::Value> value)
{
	while (value->IsObject())
	{
		v8::Local<v8::Object> obj = value->ToObject();
		T native = reinterpret_cast<T>(obj->GetAlignedPointerFromInternalField(0));
		if (native)
		{
			return native;
		}
		value = obj->GetPrototype();
	}
	return nullptr;
}

// A string that converts to char const * (useful for fusion::invoke)
template<typename Char>
struct convertible_string : std::basic_string<Char>
{
	convertible_string(Char const *str, size_t len) : std::basic_string<Char>(str, len) {}

	operator Char const*() const { return this->c_str(); }
};

template<typename T>
struct from_v8;

template<>
struct from_v8<std::string>
{
	typedef std::string result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type const& def_value)
	{
		return value->IsString()? exec(value) :  def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsString())
		{
			v8::String::Utf8Value const utf8_str(value);
			return result_type(*utf8_str, utf8_str.length());
		}
		throw std::runtime_error("expected String");
	}
};

template<>
struct from_v8<convertible_string<char>>
{
	typedef convertible_string<char> result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type const& def_value)
	{
		return value->IsString()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsString())
		{
			v8::String::Utf8Value const utf8_str(value);
			return result_type(*utf8_str, utf8_str.length());
		}
		throw std::runtime_error("expected String");
	}
};

// char const * and char const * have to be copied immediately otherwise
// the underlying memory will die due to the way v8 strings work.
template<>
struct from_v8<char const *> : from_v8<convertible_string<char>> {};

template<>
struct from_v8<char const * const> : from_v8<convertible_string<char>> {};

#ifdef WIN32
static_assert(sizeof(wchar_t) == sizeof(uint16_t), "wchar_t has 16 bits");

template<>
struct from_v8<std::wstring>
{
	typedef std::wstring result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type const& def_value)
	{
		return value->IsString()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsString())
		{
			v8::String::Value const utf16_str(value);
			return result_type((wchar_t const*)(*utf16_str), utf16_str.length());
		}
		throw std::runtime_error("expected String");
	}
};

template<>
struct from_v8<convertible_string<wchar_t>>
{
	typedef convertible_string<wchar_t> result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type const& def_value)
	{
		return value->IsString()? exec(value) :def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsString())
		{
			v8::String::Value const utf16_str(value);
			return result_type((wchar_t const*)(*utf16_str), utf16_str.length());
		}
		throw std::runtime_error("expected String");
	}
};

template<>
struct from_v8<wchar_t const *> : from_v8<convertible_string<wchar_t>> {};

template<>
struct from_v8<wchar_t const * const> : from_v8<convertible_string<wchar_t>> {};
#endif

template<>
struct from_v8< v8::Handle<v8::Function> >
{
	typedef v8::Handle<v8::Function> result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsFunction()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsFunction())
		{
			return value.As<v8::Function>();
		}
		throw std::runtime_error("expected Function");
	}
};

template<>
struct from_v8<bool>
{
	typedef bool result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsBoolean()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		return value->ToBoolean()->Value();
	}
};

template<>
struct from_v8<int32_t>
{
	typedef int32_t result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsNumber()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsNumber())
		{
			return value->ToInt32()->Value();
		}
		throw std::runtime_error("expected Number");
	}
};

template<>
struct from_v8<uint32_t>
{
	typedef uint32_t result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsNumber()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsNumber())
		{
			return value->ToUint32()->Value();
		}
		throw std::runtime_error("expected Number");
	}
};

template<>
struct from_v8<int64_t>
{
	typedef int64_t result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsNumber()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsNumber())
		{
			return static_cast<result_type>(value->ToNumber()->Value());
		}
		throw std::runtime_error("expected Number");
	}
};

template<>
struct from_v8<uint64_t>
{
	typedef uint64_t result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsNumber()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsNumber())
		{
			return static_cast<result_type>(value->ToNumber()->Value());
		}
		throw std::runtime_error("expected Number");
	}
};

template<>
struct from_v8<float>
{
	typedef float result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsNumber()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsNumber())
		{
			return static_cast<float>(value->ToNumber()->Value());
		}
		throw std::runtime_error("expected Number");
	}
};

template<>
struct from_v8<double>
{
	typedef double result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsNumber()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsNumber())
		{
			return value->ToNumber()->Value();
		}
		throw std::runtime_error("expected Number");
	}
};

template<>
struct from_v8<float>
{
	typedef float result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsNumber() )
		{
			throw std::runtime_error("expected javascript number");
		}
		return static_cast<float>(value->ToNumber()->Value());
	}
};

template<typename T, class A>
struct from_v8< std::vector<T, A> >
{
	typedef std::vector<T, A> result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type const& def_value)
	{
		return value->IsArray()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsArray())
		{
			v8::Array* array = v8::Array::Cast(*value);

			result_type result;
			result.reserve(array->Length());
			for (uint32_t i = 0, count = array->Length(); i < count; ++i)
			{
				result.push_back(from_v8<T>::exec(array->Get(i)));
			}
			return result;
		}
		throw std::runtime_error("expected Array");
	}
};

template<>
struct from_v8< v8::Handle<v8::Value> >
{
	typedef v8::Handle<v8::Value> result_type;

	static result_type exec(v8::Handle<v8::Value> value, v8::Handle<v8::Value> def_value)
	{
		return value.IsEmpty()? def_value : value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		return value;
	}
};

////////////////////////////////////////////////////////////////////////////
// extracting classes
template<typename T>
struct from_v8_ptr
{
	typedef T* result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsObject()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsObject())
		{
			if (T* obj_ptr = get_object_field<T*>(value))
			{
				return obj_ptr;
			}
			throw std::runtime_error(std::string("expected C++ wrapped object ") + typeid(T).name());
		}
		return nullptr;
	}
};

template<typename T>
struct from_v8<T*> : from_v8_ptr<T> {};

template<typename T>
struct from_v8<T const *> : from_v8_ptr<T const> {};

//////////////////////////////////////////////////////////////////////////////
// deal with references
template<typename T, typename U>
struct from_v8_ref
{
	typedef U result_type;

	static result_type exec(v8::Handle<v8::Value> value, result_type def_value)
	{
		return value->IsObject()? exec(value) : def_value;
	}

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if (value->IsObject())
		{
			if (T* obj_ptr = get_object_field<T*>(value))
			{
				return *obj_ptr;
			}
		}
		throw std::runtime_error(std::string("expected C++ wrapped object ") + typeid(T).name());
	}
};

template<typename U>
struct from_v8_ref<std::string, U> : from_v8<std::string> {};
#ifdef WIN32
template<typename U>
struct from_v8_ref<std::wstring, U> : from_v8<std::wstring> {};
#endif

template<typename T, typename U, typename R>
struct from_v8_ref<std::vector<T, U>, R> : from_v8< std::vector<T, U> > {};

template<typename T>
struct from_v8<T const &> : from_v8_ref<T, T const&> {};

template<typename T>
struct from_v8<T&> : from_v8_ref<T, T&> {};

} // namespace detail

template<typename T>
inline typename detail::from_v8<T>::result_type from_v8(v8::Handle<v8::Value> src)
{
	return detail::from_v8<T>::exec(src);
}

template<typename T>
inline typename detail::from_v8<T>::result_type from_v8(v8::Handle<v8::Value> src, T const& def_value)
{
	return detail::from_v8<T>::exec(src, def_value);
}

} // namesapce v8pp

#endif // V8PP_FROM_V8_HPP_INCLUDED
