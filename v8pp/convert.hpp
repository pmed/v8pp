#ifndef V8PP_CONVERT_HPP_INCLUDED
#define V8PP_CONVERT_HPP_INCLUDED

#include <v8.h>

#include <climits>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "v8pp/ptr_traits.hpp"
#include "v8pp/utility.hpp"

namespace v8pp {

template<typename T, typename Traits>
class class_;

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
template<typename String>
struct convert<String, typename std::enable_if<detail::is_string<String>::value>::type>
{
	using Char = typename String::value_type;
	using Traits = typename String::traits_type;

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
		return !value.IsEmpty() && (value->IsString() || value->IsNumber());
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "String");
		}

		if (sizeof(Char) == 1)
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

	static to_type to_v8(v8::Isolate* isolate, basic_string_view<Char, Traits> value)
	{
		if (sizeof(Char) == 1)
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

// converter specializations for null-terminated strings
template<>
struct convert<char const*> : convert<basic_string_view<char>>
{
};

template<>
struct convert<char16_t const*> : convert<basic_string_view<char16_t>>
{
};

#ifdef WIN32
template<>
struct convert<wchar_t const*> : convert<basic_string_view<wchar_t>>
{
};
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

	enum { bits = sizeof(T) * CHAR_BIT, is_signed = std::is_signed<T>::value };

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

		if (bits <= 32)
		{
			if (is_signed)
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
		if (bits <= 32)
		{
			if (is_signed)
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

// convert std::tuple <-> Array
template<typename... Ts>
struct convert<std::tuple<Ts...>>
{
	using from_type = std::tuple<Ts...>;
	using to_type = v8::Local<v8::Array>;

	static constexpr size_t N = sizeof...(Ts);

	static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
	{
		return !value.IsEmpty() && value->IsArray()
			&& value.As<v8::Array>()->Length() == N;
	}

	static from_type from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	{
		if (!is_valid(isolate, value))
		{
			throw invalid_argument(isolate, value, "Tuple");
		}
		return from_v8_impl(isolate, value, std::make_index_sequence<N>{});
	}

	static to_type to_v8(v8::Isolate* isolate, from_type const& value)
	{
		return to_v8_impl(isolate, value, std::make_index_sequence<N>{});
	}

private:
	template<size_t... Is>
	static from_type from_v8_impl(v8::Isolate* isolate, v8::Local<v8::Value> value,
		std::index_sequence<Is...>)
	{
		v8::HandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Array> array = value.As<v8::Array>();

		return std::tuple<Ts...>{ v8pp::convert<Ts>::from_v8(isolate, array->Get(context, Is).ToLocalChecked())... };
	}

	template<size_t... Is>
	static to_type to_v8_impl(v8::Isolate* isolate, std::tuple<Ts...> const& value, std::index_sequence<Is...>)
	{
		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Array> result = v8::Array::New(isolate, N);

		(void)std::initializer_list<bool>{ result->Set(context, Is, convert<Ts>::to_v8(isolate, std::get<Is>(value))).FromJust()... };

		return scope.Escape(result);
	}
};

// convert Array <-> std::array, vector, deque, list
template<typename Sequence>
struct convert<Sequence, typename std::enable_if<detail::is_sequence<Sequence>::value || detail::is_array<Sequence>::value>::type>
{
	using from_type = Sequence;
	using to_type = v8::Local<v8::Array>;
	using item_type = typename Sequence::value_type;

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

		from_type result{};

		using is_array = detail::is_array<Sequence>;
		is_array::check_length(array->Length());
		detail::has_reserve<Sequence>::reserve(result, array->Length());

		for (uint32_t i = 0, count = array->Length(); i < count; ++i)
		{
			v8::Local<v8::Value> item = array->Get(context, i).ToLocalChecked();
			is_array::set_element_at(result, i, convert<item_type>::from_v8(isolate, item));
		}
		return result;
	}

	static to_type to_v8(v8::Isolate* isolate, from_type const& value)
	{
		constexpr int max_size = std::numeric_limits<int>::max();
		if (value.size() > max_size)
		{
			throw std::runtime_error("Invalid array length: actual "
				+ std::to_string(value.size()) + " exceeds maximal "
				+ std::to_string(max_size));
		}

		v8::EscapableHandleScope scope(isolate);
		v8::Local<v8::Context> context = isolate->GetCurrentContext();
		v8::Local<v8::Array> result = v8::Array::New(isolate, static_cast<int>(value.size()));
		uint32_t i = 0;
		for (item_type const& item : value)
		{
			result->Set(context, i++, convert<item_type>::to_v8(isolate, item)).FromJust();
		}
		return scope.Escape(result);
	}
};

// convert Object <-> std::{unordered_}{multi}map
template<typename Mapping>
struct convert<Mapping, typename std::enable_if<detail::is_mapping<Mapping>::value>::type>
{
	using from_type = Mapping;
	using to_type = v8::Local<v8::Object>;

	using Key = typename Mapping::key_type;
	using Value = typename Mapping::mapped_type;

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
		for (auto const& item : value)
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
struct is_wrapped_class : conjunction<
	std::is_class<T>,
	negation<detail::is_string<T>>,
	negation<detail::is_mapping<T>>,
	negation<detail::is_sequence<T>>,
	negation<detail::is_array<T>>,
	negation<detail::is_tuple<T>>,
	negation<detail::is_shared_ptr<T>>
>
{
};

// convert specialization for wrapped user classes
template<typename T>
struct is_wrapped_class<v8::Local<T>> : std::false_type
{
};

template<typename T>
struct is_wrapped_class<v8::Global<T>> : std::false_type
{
};

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
			//assert(object.use_count() > 1);
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
struct convert<T&> : convert<T>
{
};

template<typename T>
struct convert<T const&> : convert<T>
{
};

template<typename T>
auto from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value)
	-> decltype(convert<T>::from_v8(isolate, value))
{
	return convert<T>::from_v8(isolate, value);
}

template<typename T, typename U>
auto from_v8(v8::Isolate* isolate, v8::Local<v8::Value> value, U const& default_value)
	-> decltype(convert<T>::from_v8(isolate, value))
{
	return convert<T>::is_valid(isolate, value) ?
		convert<T>::from_v8(isolate, value) : default_value;
}

inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, char const* str, size_t len)
{
	return convert<string_view>::to_v8(isolate, string_view(str, len));
}

template<size_t N>
v8::Local<v8::String> to_v8(v8::Isolate* isolate,
	char const (&str)[N], size_t len = N - 1)
{
	return convert<string_view>::to_v8(isolate, string_view(str, len));
}

inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, char16_t const* str, size_t len)
{
	return convert<u16string_view>::to_v8(isolate, u16string_view(str, len));
}

template<size_t N>
v8::Local<v8::String> to_v8(v8::Isolate* isolate,
	char16_t const (&str)[N], size_t len = N - 1)
{
	return convert<u16string_view>::to_v8(isolate, u16string_view(str, len));
}

#ifdef WIN32
inline v8::Local<v8::String> to_v8(v8::Isolate* isolate, wchar_t const* str, size_t len)
{
	return convert<wstring_view>::to_v8(isolate, wstring_view(str, len));
}

template<size_t N>
v8::Local<v8::String> to_v8(v8::Isolate* isolate,
	wchar_t const (&str)[N], size_t len = N - 1)
{
	return convert<wstring_view>::to_v8(isolate, wstring_view(str, len));
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

template<typename... Ts>
v8::Local<v8::Array> to_v8(v8::Isolate* isolate, std::tuple<Ts...> const& value)
{
	return convert<std::tuple<Ts...>>::to_v8(isolate, value);
}

template<typename T>
v8::Local<v8::Array> to_v8(v8::Isolate* isolate, std::initializer_list<T> const& init)
{
	return to_v8(isolate, init.begin(), init.end());
}

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
