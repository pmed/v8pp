#ifndef V8PP_TO_V8_HPP_INCLUDED
#define V8PP_TO_V8_HPP_INCLUDED

#include <v8.h>

#include <string>
#include <cstdint>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

namespace v8pp {

namespace detail {

// Native objects registry. Monostate
template<typename T>
class object_registry
{
public:
#if V8PP_USE_GLOBAL_OBJECTS_REGISTRY
	typedef boost::unordered_map<void const*, v8::Persistent<v8::Value>> objects;

	// use additional set to distiguish instances of T in the global registry
	static boost::unordered_set<T*>& instances()
	{
		static boost::unordered_set<T*> instances_;
		return instances_;
	}

#else
	typedef boost::unordered_map<T const*, v8::Persistent<v8::Value>> objects;
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
		objects::iterator it = items().find(object);
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
		objects::iterator it = items().find(native);
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

} // detail

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
	return detail::object_registry<T>::find(src);
}

template<typename T>
typename boost::enable_if<boost::is_class<T>, v8::Handle<v8::Value> >::type to_v8(T const& src)
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
