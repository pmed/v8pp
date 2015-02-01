#ifndef V8PP_FACTORY_HPP_INCLUDED
#define V8PP_FACTORY_HPP_INCLUDED

#include <utility>

#include <v8.h>

namespace v8pp {

// Factory that calls C++ constructor
template<typename T>
struct factory
{
	static size_t const object_size = sizeof(T);

	template<typename ...Args>
	static T* create(v8::Isolate* isolate, Args... args)
	{
		T* object = new T(std::forward<Args>(args)...);
		isolate->AdjustAmountOfExternalAllocatedMemory(static_cast<int64_t>(object_size));
		return object;
	}

	static void destroy(v8::Isolate* isolate, T* object)
	{
		delete object;
		isolate->AdjustAmountOfExternalAllocatedMemory(-static_cast<int64_t>(object_size));
	}
};

} //namespace v8pp

#endif // V8PP_FACTORY_HPP_INCLUDED
