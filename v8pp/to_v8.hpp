#ifndef V8PP_TO_V8_HPP_INCLUDED
#define V8PP_TO_V8_HPP_INCLUDED

#include <v8.h>

#include <string>
#include <cstdint>

#include <boost/unordered_map.hpp>

namespace v8pp {

namespace detail {

// Native objects registry. Monostate
class object_registry
{
public:
	typedef boost::unordered_map<void const*, v8::Persistent<v8::Value>> objects;

	static void add(void const* object, v8::Handle<v8::Value> value)
	{
		items_.insert(objects::value_type(object, v8::Persistent<v8::Value>::New(value)));
	}

	static void remove(void const* object)
	{
		objects::iterator it = items_.find(object);
		if ( it != items_.end() )
		{
			it->second.Dispose();
			items_.erase(it);
		}
	}

	static v8::Handle<v8::Value> find(void const* native)
	{
		v8::Handle<v8::Value> result;
		objects::iterator it = items_.find(native);
		if ( it != items_.end() )
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
	static objects items_;
};

} // namespace detail

template<typename T>
inline v8::Handle<T> to_v8(v8::Handle<T> src)
{
	return src;
}

inline v8::Handle<v8::Value> to_v8(std::string const& src)
{
	return v8::String::New(src.data(), src.length());
}

inline v8::Handle<v8::Value> to_v8(char const *src)
{
	return v8::String::New(src? src : "");
}

#ifdef WIN32
static_assert(sizeof(wchar_t) == sizeof(uint16_t), "wchar_t has 16 bits");

inline v8::Handle<v8::Value> to_v8(std::wstring const& src)
{
	return v8::String::New((uint16_t const*)src.data(), src.length());
}

inline v8::Handle<v8::Value> to_v8(wchar_t const *src)
{
	return v8::String::New((uint16_t const*)(src? src : L""));
}
#endif

inline v8::Handle<v8::Value> to_v8(int64_t const src)
{
	// TODO: check bounds?
	return v8::Number::New(static_cast<double>(src));
}

inline v8::Handle<v8::Value> to_v8(uint64_t const src)
{
	// TODO: check bounds?
	return v8::Number::New(static_cast<double>(src));
}

inline v8::Handle<v8::Value> to_v8(float const src)
{
	return v8::Number::New(src);
}

inline v8::Handle<v8::Value> to_v8(double const src)
{
	return v8::Number::New(src);
}

inline v8::Handle<v8::Value> to_v8(int32_t const src)
{
	return v8::Int32::New(src);
}

inline v8::Handle<v8::Value> to_v8(uint32_t const src)
{
	return v8::Uint32::New(src);
}

inline v8::Handle<v8::Value> to_v8(bool const src)
{
	return v8::Boolean::New(src);
}

template<typename T>
typename boost::enable_if<boost::is_enum<T>, v8::Handle<v8::Value> >::type to_v8(T const src)
{
	return v8::Int32::New(src);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type to_v8(T const* src)
{
	return detail::object_registry::find(src);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type to_v8(T* src)
{
	return to_v8(static_cast<T const*>(src));
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type to_v8(T const& src)
{
	return to_v8(&src);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type to_v8(T& src)
{
	return to_v8(&src);
}

template<typename ForwardIterator>
v8::Handle<v8::Value> to_v8(ForwardIterator begin, ForwardIterator end)
{
	v8::HandleScope scope;

	v8::Handle<v8::Array> result = v8::Array::New();
	for (uint32_t idx = 0; begin != end; ++begin, ++idx)
	{
		result->Set(idx, to_v8(*begin));
	}
	return scope.Close(result);
}

template<typename T>
v8::Handle<v8::Value> to_v8(std::vector<T> const& src)
{
	return to_v8(src.begin(), src.end());
}

} // namespace v8pp

#endif // V8PP_TO_V8_HPP_INCLUDED
