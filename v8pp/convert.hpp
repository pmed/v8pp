//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_CONVERT_HPP_INCLUDED
#define V8PP_CONVERT_HPP_INCLUDED

#include <v8.h>

#include <climits>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <memory>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <variant>
#include <optional>
#include "v8pp/ptr_traits.hpp"
#include "v8pp/utility.hpp"
#include "math.h"



namespace v8pp {

template<typename T, typename Traits>
class class_;

namespace detail {
template<typename Traits>
class object_registry;
}

// Generic convertor
/*
template<typename T, typename Enable = void>
struct convert
{
	using from_type = T;
	using to_type = v8::Local<v8::Value>;

	static bool is_valid(v8::Isolate* isolate, v8::Local<v8::Value> value);

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value);
	static to_type to_v8(v8::Isolate* isolate, T const& value);
};
*/

struct invalid_argument : std::invalid_argument
{
	invalid_argument(v8::Isolate* isolate, v8::Local<v8::Value> value, char const* expected_type);
};

// converter specializations for string types
template<typename Char, typename Traits>
struct convert<std::basic_string_view<Char, Traits>>
{
	static_assert(sizeof(Char) <= sizeof(uint16_t),
		"only UTF-8 and UTF-16 strings are supported");

	// A string that converts to Char const*
	struct convertible_string : std::basic_string<Char, Traits>
	{
		using base_class = std::basic_string<Char, Traits>;
		using base_class::base_class;
		operator Char const*() const { return this->c_str(); }
	};

	using from_type = convertible_string;
	using to_type = v8::Local<v8::String>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsString();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "String");
		}

		if constexpr (sizeof(Char) == 1)
		{
			v8::String::Utf8Value const str(isolate, value);
			return from_type(reinterpret_cast<Char const*>(*str), str.length());
		}
		else
		{
			v8::String::Value const str(isolate, value);
			return from_type(reinterpret_cast<Char const*>(*str), str.length());
		}
	}

	static to_type to_v8(v8::Isolate* isolate, std::basic_string_view<Char, Traits> value)
	{
		if constexpr (sizeof(Char) == 1)
		{
			return v8::String::NewFromUtf8(isolate,
				reinterpret_cast<char const*>(value.data()),
				v8::NewStringType::kNormal, static_cast<int>(value.size())).ToLocalChecked();
		}
		else
		{
			return v8::String::NewFromTwoByte(isolate,
				reinterpret_cast<uint16_t const*>(value.data()),
				v8::NewStringType::kNormal, static_cast<int>(value.size())).ToLocalChecked();
		}
	}
};

template<typename Char, typename Traits, typename Alloc>
struct convert<std::basic_string<Char, Traits, Alloc>> : convert<std::basic_string_view<Char, Traits>>
{
};

// converter specializations for null-terminated strings
template<>
struct convert<char const*> : convert<std::basic_string_view<char>> {};
template<>
struct convert<char16_t const*> : convert<std::basic_string_view<char16_t>> {};
#ifdef WIN32
template<>
struct convert<wchar_t const*> : convert<std::basic_string_view<wchar_t>> {};
#endif

// converter specializations for primitive types
template<>
struct convert<bool>
{
	using from_type = bool;
	using to_type = v8::Local<v8::Boolean>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsBoolean();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Boolean");
		}
#if (V8_MAJOR_VERSION > 7) || (V8_MAJOR_VERSION == 7 && V8_MINOR_VERSION >= 1)
		return value->BooleanValue(isolate);
#else
		return value->BooleanValue(isolate->GetCurrentContext()).FromJust();
#endif
	}

	static to_type to_v8(v8::Isolate* isolate, bool value)
	{
		return v8::Boolean::New(isolate, value);
	}
};

template<typename T>
struct convert<T, typename std::enable_if<std::is_integral<T>::value>::type>
{
	using from_type = T;
	using to_type = v8::Local<v8::Number>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsNumber();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Number");
		}

		if constexpr (sizeof(T) <= sizeof(uint32_t))
		{
			if constexpr (std::is_signed_v<T>)
			{
				return static_cast<T>(value->Int32Value(isolate->GetCurrentContext()).FromJust());
			}
			else
			{
				return static_cast<T>(value->Uint32Value(isolate->GetCurrentContext()).FromJust());
			}
		}
		else
		{
			return static_cast<T>(value->IntegerValue(isolate->GetCurrentContext()).FromJust());
		}
	}

	static to_type to_v8(v8::Isolate* isolate, T value)
	{
		if constexpr (sizeof(T) <= sizeof(uint32_t)) 
		{
			if constexpr (std::is_signed_v<T>)
			{
				return v8::Integer::New(isolate,
					static_cast<int32_t>(value));
			}
			else
			{
				return v8::Integer::NewFromUnsigned(isolate,
					static_cast<uint32_t>(value));
			}
		}
		else
		{
			//TODO: check value < (1<<57) to fit in double?
			return v8::Number::New(isolate, static_cast<double>(value));
		}
	}
};

template<typename T>
struct convert<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
	using underlying_type = typename std::underlying_type<T>::type;

	using from_type = T;
	using to_type = typename convert<underlying_type>::to_type;

	static bool is_valid(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		return convert<underlying_type>::is_valid(isolate, value);
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		return static_cast<T>(convert<underlying_type>::from_v8(isolate, value));
	}

	static to_type to_v8(v8::Isolate* isolate, T value)
	{
		return convert<underlying_type>::to_v8(isolate,
			static_cast<underlying_type>(value));
	}
};

template<typename T>
struct convert<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
{
	using from_type = T;
	using to_type = v8::Local<v8::Number>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsNumber();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Number");
		}

		return static_cast<T>(value->NumberValue(isolate->GetCurrentContext()).FromJust());
	}

	static to_type to_v8(v8::Isolate* isolate, T value)
	{
		return v8::Number::New(isolate, value);
	}
};

// convert Array <-> std::array
template<typename T, size_t N>
struct convert<std::array<T, N>>
{
	using from_type = std::array<T, N>;
	using to_type = v8::Local<v8::Array>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsArray();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Array");
		}

		v8::HandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Array> array = value.As<v8::Array>();

		if (array->Length() != N)
		{
			throw std::runtime_error("Invalid array length: expected "
				+ std::to_string(N) + " actual "
				+ std::to_string(array->Length()));
		}

		from_type result = {};
		for (uint32_t i = 0; i < N; ++i)
		{
			result[i] = convert<T>::from_v8(isolate, array->Get(context, i).ToLocalChecked());
		}
		return result;
	}

	static to_type to_v8(v8::Isolate* isolate, from_type const& value)
	{
		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Array> result = v8::Array::New(isolate, N);
		for (uint32_t i = 0; i < N; ++i)
		{
			result->Set(context, i, convert<T>::to_v8(isolate, value[i])).FromJust();
		}
		return scope.Escape(result);
	}
};

// convert Array <-> std::vector
template<typename T, typename Alloc>
struct convert<std::vector<T, Alloc>>
{
	using from_type = std::vector<T, Alloc>;
	using to_type = v8::Local<v8::Array>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsArray();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Array");
		}

		v8::HandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Array> array = value.As<v8::Array>();

		from_type result;
		result.reserve(array->Length());
		for (uint32_t i = 0, count = array->Length(); i < count; ++i)
		{
			result.emplace_back(convert<T>::from_v8(isolate, array->Get(context, i).ToLocalChecked()));
		}
		return result;
	}

	static to_type to_v8(v8::Isolate* isolate, from_type const& value)
	{
		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		uint32_t const size = static_cast<uint32_t>(value.size());
		v8::Local<v8::Array> result = v8::Array::New(isolate, size);
		for (uint32_t i = 0; i < size; ++i)
		{
			result->Set(context, i, convert<T>::to_v8(isolate, value[i])).FromJust();
		}
		return scope.Escape(result);
	}
};

// convert Object <-> std::map
template<typename Key, typename Value, typename Less, typename Alloc>
struct convert<std::map<Key, Value, Less, Alloc>>
{
	using from_type = std::map<Key, Value, Less, Alloc>;
	using to_type = v8::Local<v8::Object>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsObject();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Object");
		}

		v8::HandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Object> object = value.As<v8::Object>();
		v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();

		from_type result;
		for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
		{
			v8::Local<v8::Value> key = prop_names->Get(context, i).ToLocalChecked();
			v8::Local<v8::Value> val = object->Get(context, key).ToLocalChecked();
			result.emplace(convert<Key>::from_v8(isolate, key),
				convert<Value>::from_v8(isolate, val));
		}
		return result;
	}

	static to_type to_v8(v8::Isolate* isolate, from_type const& value)
	{
		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Object> result = v8::Object::New(isolate);
		for (auto const& item: value)
		{
			result->Set(context,
				convert<Key>::to_v8(isolate, item.first),
				convert<Value>::to_v8(isolate, item.second)).FromJust();
		}
		return scope.Escape(result);
	}
};

template<typename T>
struct convert<v8::Local<T>>
{
	using from_type = v8::Local<T>;
	using to_type = v8::Local<T>;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.As<T>().IsEmpty();
	}

	static v8::Local<T> from_v8(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return value.As<T>();
	}

	static v8::Local<T> to_v8(v8::Isolate*, v8::Local<T> value)
	{
		return value;
	}
};


template<typename T>
struct is_wrapped_class : std::is_class<T> {};

// convert specialization for wrapped user classes
template<typename T>
struct is_wrapped_class<v8::Local<T>> : std::false_type {};

template<typename T>
struct is_wrapped_class<v8::Global<T>> : std::false_type {};

template<typename Char, typename Traits, typename Alloc>
struct is_wrapped_class<std::basic_string<Char, Traits, Alloc>> : std::false_type {};

template<typename Char, typename Traits>
struct is_wrapped_class<std::basic_string_view<Char, Traits>> : std::false_type {};

template<typename T, size_t N>
struct is_wrapped_class<std::array<T, N>> : std::false_type{};

template <typename ... Ts>
struct is_wrapped_class<std::variant<Ts...>> : std::false_type {};

template<typename T, typename Alloc>
struct is_wrapped_class<std::vector<T, Alloc>> : std::false_type {};

template<typename Key, typename Value, typename Less, typename Alloc>
struct is_wrapped_class<std::map<Key, Value, Less, Alloc>> : std::false_type {};

template<typename T>
struct is_wrapped_class<std::shared_ptr<T>> : std::false_type {};

template<typename T>
struct convert<T*, typename std::enable_if<is_wrapped_class<T>::value>::type>
{
	using from_type = T*;
	using to_type = v8::Local<v8::Object>;
	using class_type = typename std::remove_cv<T>::type;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsObject();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			return nullptr;
		}
		return class_<class_type, raw_ptr_traits>::unwrap_object(isolate, value);
	}

	static to_type to_v8(v8::Isolate* isolate, T const* value)
	{
		return class_<class_type, raw_ptr_traits>::find_object(isolate, value);
	}
};

template<typename T>
struct convert<T, typename std::enable_if<is_wrapped_class<T>::value>::type>
{
	using from_type = T&;
	using to_type = v8::Local<v8::Object>;
	using class_type = typename std::remove_cv<T>::type;

	static bool is_valid(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		return convert<T*>::is_valid(isolate, value);
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Object");
		}
		T* object = class_<class_type, raw_ptr_traits>::unwrap_object(isolate, value);
		if (object)
		{
			return *object;
		}
		throw std::runtime_error("failed to unwrap C++ object");
	}

	static to_type to_v8(v8::Isolate* isolate, T const& value)
	{
		v8::Local<v8::Object> result = class_<class_type, raw_ptr_traits>::find_object(isolate, value);
		if (!result.IsEmpty()) return result;
		throw std::runtime_error("failed to wrap C++ object");
	}
};

template<typename T>
struct convert<std::shared_ptr<T>, typename std::enable_if<is_wrapped_class<T>::value>::type>
{
	using from_type = std::shared_ptr<T>;
	using to_type = v8::Local<v8::Object>;
	using class_type = typename std::remove_cv<T>::type;

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsObject();
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			return nullptr;
		}
		return class_<class_type, shared_ptr_traits>::unwrap_object(isolate, value);
	}

	static to_type to_v8(v8::Isolate* isolate, std::shared_ptr<T> const& value)
	{
		return class_<class_type, shared_ptr_traits>::find_object(isolate, value);
	}
};

template<typename T>
struct convert<T, ref_from_shared_ptr>
{
	using from_type = T&;
	using to_type = v8::Local<v8::Object>;
	using class_type = typename std::remove_cv<T>::type;

	static bool is_valid(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		return convert<std::shared_ptr<T>>::is_valid(isolate, value);
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Object");
		}
		std::shared_ptr<T> object = class_<class_type, shared_ptr_traits>::unwrap_object(isolate, value);
		if (object)
		{
//			assert(object.use_count() > 1);
			return *object;
		}
		throw std::runtime_error("failed to unwrap C++ object");
	}

	static to_type to_v8(v8::Isolate* isolate, T const& value)
	{
		v8::Local<v8::Object> result = class_<class_type, shared_ptr_traits>::find_object(isolate, value);
		if (!result.IsEmpty()) return result;
		throw std::runtime_error("failed to wrap C++ object");
	}
};

template<typename T>
struct convert<T&> : convert<T> {};

template<typename T>
struct convert<T const&> : convert<T> {};

template<typename T>
auto from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	-> decltype(convert<T>::from_v8(isolate, value))
{
	return convert<T>::from_v8(isolate, value);
}

template<typename T, typename U>
auto from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value,U const& default_value)
	-> decltype(convert<T>::from_v8(isolate, value))
{
	using return_type = decltype(convert<T>::from_v8(isolate, value));
	return convert<T>::is_valid(isolate, value)?
		convert<T>::from_v8(isolate, value) : static_cast<return_type>(default_value);
}

inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, char const* str)
{
	return convert<std::string_view>::to_v8(isolate, std::string_view(str));
}

inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, char const* str, size_t len)
{
	return convert<std::string_view>::to_v8(isolate, std::string_view(str, len));
}

template<size_t N>
v8::Local<v8::String> to_v8(v8::Isolate* isolate,
	char const (&str)[N], size_t len = N - 1)
{
	return convert<std::string_view>::to_v8(isolate, std::string_view(str, len));
}

inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, char16_t const* str)
{
	return convert<std::u16string_view>::to_v8(isolate, std::u16string_view(str));
}

inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, char16_t const* str, size_t len)
{
	return convert<std::u16string_view>::to_v8(isolate, std::u16string_view(str, len));
}

template<size_t N>
v8::Local<v8::String> to_v8(v8::Isolate* isolate,
	char16_t const (&str)[N], size_t len = N - 1)
{
	return convert<std::u16string_view>::to_v8(isolate, std::u16string_view(str, len));
}

#ifdef WIN32
inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, wchar_t const* str)
{
	return convert<std::wstring_view>::to_v8(isolate, std::wstring_view(str));
}

inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, wchar_t const* str, size_t len)
{
	return convert<std::wstring_view>::to_v8(isolate, std::wstring_view(str, len));
}

template<size_t N>
v8::Local<v8::String> to_v8(v8::Isolate* isolate,
	wchar_t const (&str)[N], size_t len = N - 1)
{
	return convert<std::wstring_view>::to_v8(isolate, std::wstring_view(str, len));
}
#endif

template<typename T>
auto to_v8(v8::Isolate* isolate, T const& value)
	-> decltype(convert<T>::to_v8(isolate, value))
{
	return convert<T>::to_v8(isolate, value);
}

template<typename Iterator>
v8::Local<v8::Array> to_v8(v8::Isolate* isolate, Iterator begin, Iterator end)
{
	v8::EscapableHandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Array> result = v8::Array::New(isolate);
	for (uint32_t idx = 0; begin != end; ++begin, ++idx)
	{
		result->Set(context, idx, to_v8(isolate, *begin)).FromJust();
	}
	return scope.Escape(result);
}

template<typename T>
v8::Local<v8::Array> to_v8(v8::Isolate* isolate, std::initializer_list<T> const& init)
{
	return to_v8(isolate, init.begin(), init.end());
}



template <typename ... Ts>
struct convert<std::variant<Ts...>>
{
    using from_type = std::variant<Ts...>;
    using to_type = v8::Local<v8::Value>;
    static constexpr std::size_t N = sizeof ... (Ts);

    template <typename T> struct isArray : std::false_type {};
    template <typename T, typename Alloc> struct isArray<std::vector<T, Alloc>> : std::true_type {};
    template <typename T, std::size_t N> struct isArray<std::array<T, N>> : std::true_type {};
    template <typename T> struct isBoolean : std::false_type {};
    template <> struct isBoolean<bool> : std::true_type {};
    template <typename T> struct isIntegralNotBool : std::is_integral<T> {};
    template <> struct isIntegralNotBool<bool> : std::false_type {};
    template <typename T> struct isObj : v8pp::is_wrapped_class<T> {};
    template <typename T> struct isObj<std::shared_ptr<T>> : std::true_type {};
    template <typename T> struct isSharedPtr : std::false_type {};
    template <typename T> struct isSharedPtr<std::shared_ptr<T>> : std::true_type {};
    template <typename T> struct isString : std::false_type {};
    template<typename Char, typename Traits, typename Alloc> struct isString<std::basic_string<Char, Traits, Alloc>> : std::true_type {};
    template <> struct isString<const char*> : std::true_type {};
    template <> struct isString<char16_t const*> : std::true_type {};
    template <> struct isString<wchar_t const*> : std::true_type {};
    template <typename T> struct isAny : std::true_type {};


    static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
    {
        return !value.IsEmpty();
    }

    static from_type from_v8(v8::Isolate * isolate, v8::Local<v8::Value> value)
    {
        if (!is_valid(isolate, value)){
            throw invalid_argument(isolate, value, "Variant");
        }

        v8::HandleScope scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        std::optional<std::variant<Ts...>> out;
        if (value->IsObject()){
            // todo: handle std::map
            out = getObject<isObj, Ts...>(isolate, value);
        } else if (value->IsArray()){
            out = getObject<isArray, Ts...>(isolate, value);
        } else if (value->IsNumber()){
            // Note: 5.f will be converted to an integer type if available,
            // since internally v8 stores all values (including integer types) as double
            const double value_ = value->NumberValue(context).FromJust();
            if (ceil(value_) == value_){
                out = getObjectAlternate<isIntegralNotBool, std::is_floating_point, isBoolean>(isolate, value);
            } else {
                out = getObjectAlternate<std::is_floating_point, isIntegralNotBool, isBoolean>(isolate, value);
            }
        } else if (value->IsBoolean()){
            out = getObjectAlternate<isBoolean, isIntegralNotBool>(isolate, value);
        } else if (value->IsString()){
            out = getObject<isString, Ts...>(isolate, value);
        } else {
            out = getObject<isAny, Ts...>(isolate, value);
        }
        if (out){
            return *out;
        }
        throw std::runtime_error("Unable to convert argument to variant.");
    }

    static to_type to_v8(v8::Isolate* isolate, std::variant<Ts...> const& value){
        return std::visit([isolate](auto && value){
            using T = std::decay_t<decltype(value)>;
            auto out = v8pp::convert<T>::to_v8(isolate, value);
            return v8::Local<v8::Value>{out};
        }, value);
    }

private:
    template <typename T>
    bool containsObject(const T& object){ return false; }

    template <typename T, typename Traits>
    static bool containsObjectImpl(v8::Local<v8::Value> value){
        while (value->IsObject())
        {
            v8::Local<v8::Object> obj = value.As<v8::Object>();
            if (obj->InternalFieldCount() == 2)
            {
                auto id = obj->GetAlignedPointerFromInternalField(0);
                if (id){
                    auto registry_pointer = static_cast<v8pp::detail::object_registry<Traits>*>(obj->GetAlignedPointerFromInternalField(1));
                    auto ptr = registry_pointer->find_object(id, v8pp::detail::type_id<T>());
                    return ptr != nullptr;
                }
                value = obj->GetPrototype();
            }
        }
        return false;
    }


    template <typename T>
    static std::enable_if_t<v8pp::is_wrapped_class<T>::value, bool> containsObject(v8::Local<v8::Value> value){
        return containsObjectImpl<T, v8pp::raw_ptr_traits>(value);
    }

    template <typename T>
    static bool containsObject(v8::Local<v8::Value> value){
        return containsObjectImpl<T, v8pp::shared_ptr_traits>(value);
    }

    template <typename T>
    static std::optional<T> getObjectImpl(v8::Isolate* isolate, v8::Local<v8::Value> value)
    {
        if (v8pp::convert<T>::is_valid(isolate, value)){
            if constexpr (v8pp::is_wrapped_class<T>::value){
                if (containsObjectImpl<T, v8pp::raw_ptr_traits>(value)){
                    return v8pp::convert<T>::from_v8(isolate, value);
                }
            } else if constexpr (isSharedPtr<T>::value){
                using U = std::remove_pointer_t<decltype(T{}.get())>;
                if (containsObjectImpl<U, v8pp::shared_ptr_traits>(value)){
                    auto ptr = v8pp::convert<T>::from_v8(isolate, value);
                    if (ptr){
                        return ptr;
                    }
                }
            } else {
                T out = v8pp::convert<T>::from_v8(isolate, value);
                return out;
            }
        }
        return std::nullopt;
    }

    template <template <typename T> typename condition, template <typename T> typename ... conditions>
    static std::optional<std::variant<Ts...>> getObjectAlternate(v8::Isolate* isolate, v8::Local<v8::Value> value)
    {
        if (auto out = getObject<condition, Ts...>(isolate, value)){
            return out;
        }
        if constexpr (sizeof ... (conditions) > 0){
            return getObjectAlternate<conditions...>(isolate, value);
        }
        return std::nullopt;
    }

    template <template <typename T> typename  condition, typename T, typename ... Ts_>
    static std::optional<std::variant<Ts...>> getObject(v8::Isolate* isolate, v8::Local<v8::Value> value)
    {
        if constexpr (condition<T>::value){
            if (auto out = getObjectImpl<T>(isolate, value)){
                return *out;
            }
        }
        if constexpr ((sizeof ... (Ts_)) > 0) {
            return getObject<condition, Ts_...>(isolate, value);
        }
        return std::nullopt;
    }
};


template<typename T>
v8::Local<T> to_local(v8::Isolate* isolate, v8::PersistentBase<T> const& handle)
{
	if (handle.IsWeak())
	{
		return v8::Local<T>::New(isolate, handle);
	}
	else
	{
		return *reinterpret_cast<v8::Local<T>*>(
			const_cast<v8::PersistentBase<T>*>(&handle));
	}
}

inline invalid_argument::invalid_argument(v8::Isolate* isolate, v8::Local<v8::Value> value, char const* expected_type)
	: std::invalid_argument(std::string("expected ")
		+ expected_type
		+ ", typeof="
		+ (value.IsEmpty() ? std::string("<empty>") : v8pp::from_v8<std::string>(isolate, value->TypeOf(isolate))))
{
}

} // namespace v8pp

#endif // V8PP_CONVERT_HPP_INCLUDED
