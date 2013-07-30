#ifndef V8PP_TO_V8_HPP_INCLUDED
#define V8PP_TO_V8_HPP_INCLUDED

#include <v8.h>

#include <string>
#include <cstdint>

#include <boost/unordered_map.hpp>

namespace v8pp {

namespace detail {

// Native objects registry. Monostate
class native_object_registry
{
public:
	static void add(void const* native, v8::Handle<v8::Value> value)
	{
		v8::Persistent<v8::Value> pvalue = v8::Persistent<v8::Value>::New(value);
		items().insert(objects::value_type(native, pvalue));
	}

	static void remove(void const* native)
	{
		objects::iterator it = items().find(native);
		if ( it != items().end() )
		{
			it->second.Dispose();
			items().erase_return_void(it);
		}
	}

	static v8::Handle<v8::Value> find(void const* native)
	{
		return items().at(native);
	}

private:
	native_object_registry();
	~native_object_registry();
	native_object_registry(native_object_registry const&);
	native_object_registry& operator=(native_object_registry const&);

private:
	typedef boost::unordered_map<void const*, v8::Persistent<v8::Value>> objects;

	static objects& items()
	{
		static objects items_;
		return items_;
	}
};

} // namespace detail

inline v8::Handle<v8::Value> to_v8(v8::Handle<v8::Value> src)
{
	return src;
}

inline v8::Handle<v8::Value> to_v8(std::string const& src)
{
	return v8::String::New(src.c_str());
}

inline v8::Handle<v8::Value> to_v8(char const *src)
{
	return v8::String::New(src);
}

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
v8::Handle<v8::Value> to_v8(T* src)
{
	return detail::native_object_registry::find(src);
}

} // namespace v8pp

#endif // V8PP_TO_V8_HPP_INCLUDED
