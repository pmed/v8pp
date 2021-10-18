#ifndef V8PP_CALL_V8_HPP_INCLUDED
#define V8PP_CALL_V8_HPP_INCLUDED

#include <v8.h>

#include "v8pp/convert.hpp"

namespace v8pp {

/// Call a V8 function, converting C++ arguments to v8::Value arguments
/// @param isolate V8 isolate instance
/// @param func  V8 function to call
/// @param recv V8 object used as `this` in the function
/// @param args...  C++ arguments to convert to JS arguments using to_v8
template<typename... Args>
v8::Local<v8::Value> call_v8(v8::Isolate* isolate, v8::Local<v8::Function> func,
	v8::Local<v8::Value> recv, Args&&... args)
{
	v8::EscapableHandleScope scope(isolate);

	int const arg_count = sizeof...(Args);
	// +1 to allocate array for arg_count == 0
	v8::Local<v8::Value> v8_args[arg_count + 1] =
	{
		to_v8(isolate, std::forward<Args>(args))...
	};

	v8::Local<v8::Value> result;
	bool const is_empty_result = func->Call(isolate->GetCurrentContext(), recv, arg_count, v8_args).ToLocal(&result);
	(void)is_empty_result;

	return scope.Escape(result);
}

} // namespace v8pp

#endif // V8PP_CALL_V8_HPP_INCLUDED
