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
	while ( value->IsObject() )
	{
		v8::Local<v8::Object> obj = value->ToObject();
		T native = reinterpret_cast<T>(obj->GetAlignedPointerFromInternalField(0));
		if ( native )
		{
			return native;
		}
		value = obj->GetPrototype();
	}
	return nullptr;
}

// A string that converts to char const * (useful for fusion::invoke)
struct convertible_string : std::string
{
	convertible_string() {}
	convertible_string(char const *str) : std::string(str) {}

	operator char const *() { return c_str(); }
};

template<typename T>
struct from_v8;

template<>
struct from_v8<std::string>
{
	typedef std::string result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsString() )
		{
			throw std::runtime_error("cannot make string from non-string type");
		}
		return *v8::String::Utf8Value(value);
	}
};

template<>
struct from_v8<convertible_string>
{
	typedef convertible_string result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsString() )
		{
			throw std::runtime_error("cannot make string from non-string type");
		}
		return *v8::String::Utf8Value(value);
	}
};

// char const * and char const * have to be copied immediately otherwise
// the underlying memory will die due to the way v8 strings work.
template<>
struct from_v8<char const *> : from_v8<convertible_string> {};

template<>
struct from_v8<char const * const> : from_v8<convertible_string> {};

template<>
struct from_v8< v8::Handle<v8::Function> >
{
	typedef v8::Handle<v8::Function> result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsFunction() )
		{
			throw std::runtime_error("expected javascript function");
		}
		return value.As<v8::Function>();
	}
};

template<>
struct from_v8<bool>
{
	typedef bool result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		return value->ToBoolean()->Value();
	}
};

template<>
struct from_v8<int32_t>
{
	typedef int32_t result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsNumber() )
		{
			throw std::runtime_error("expected javascript number");
		}

		return value->ToInt32()->Value();
	}
};

template<>
struct from_v8<uint32_t>
{
	typedef uint32_t result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsNumber() )
		{
			throw std::runtime_error("expected javascript number");
		}

		return value->ToUint32()->Value();
	}
};

template<>
struct from_v8<int64_t>
{
	typedef int64_t result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsNumber() )
		{
			throw std::runtime_error("expected javascript number");
		}
		return static_cast<result_type>(value->ToNumber()->Value());
	}
};

template<>
struct from_v8<uint64_t>
{
	typedef uint64_t result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsNumber() )
		{
			throw std::runtime_error("expected javascript number");
		}
		return static_cast<result_type>(value->ToNumber()->Value());
	}
};

template<>
struct from_v8<double>
{
	typedef double result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsNumber() )
		{
			throw std::runtime_error("expected javascript number");
		}
		return value->ToNumber()->Value();
	}
};

template<typename T, class A>
struct from_v8< std::vector<T, A> >
{
	typedef std::vector<T, A> result_type;

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsArray() )
		{
			throw std::runtime_error("expected javascript array");
		}

		v8::Array* array = v8::Array::Cast(*value);

		result_type result;
		result.reserve(array->Length());
		for (uint32_t i = 0, count = array->Length(); i < count; ++i)
		{
			result.push_back(from_v8<T>::exec(array->Get(i)));
		}
		return result;
	}
};

template<>
struct from_v8< v8::Handle<v8::Value> >
{
	typedef v8::Handle<v8::Value> result_type;

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

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsObject() )
		{
			return nullptr;
		}

		T* obj_ptr = get_object_field<T*>(value);
		if ( !obj_ptr )
		{
			throw std::runtime_error("expected C++ wrapped object");
		}
		return obj_ptr;
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

	static result_type exec(v8::Handle<v8::Value> value)
	{
		if ( !value->IsObject() )
		{
			throw std::runtime_error("expected object");
		}

		T* obj_ptr = get_object_field<T*>(value);
		if ( !obj_ptr )
		{
			throw std::runtime_error("expected C++ wrapped object");
		}
		return *obj_ptr;
	}
};

template<typename U>
struct from_v8_ref<std::string, U> : from_v8<std::string> {};

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

} // namesapce v8pp

#endif // V8PP_FROM_V8_HPP_INCLUDED
