#ifndef BOOST_PP_IS_ITERATING
#   ifndef V8PP_CALL_V8_HPP_INCLUDED
#   define V8PP_CALL_V8_HPP_INCLUDED
#       include <boost/preprocessor/repetition.hpp>
#       include <boost/preprocessor/iteration/iterate.hpp>

#       include <v8.h>

#       include "v8pp/config.hpp"
#       include "v8pp/convert.hpp"

#       ifndef V8PP_CALL_V8_MAX_SIZE
#         define V8PP_CALL_V8_MAX_SIZE V8PP_MAX_ARG_LIMIT
#       endif

namespace v8pp {

// Calls a V8 function, converting C++ arguments to v8::Value arguments
// @tparam T... C++ types of arguments
// @param func  v8 function to call
// @param args...  C++ arguments to convert to JS arguments using ToV8
inline v8::Handle<v8::Value> call_v8(v8::Handle<v8::Function> func, v8::Handle<v8::Object> recv)
{
	return func->Call(recv, 0, nullptr);
}

inline v8::Handle<v8::Value> call_v8(v8::Handle<v8::Function> func)
{
	return func->Call(func, 0, nullptr);
}

} // namespace v8pp

#       define BOOST_PP_ITERATION_LIMITS (1, V8PP_CALL_V8_MAX_SIZE - 1)
#       define BOOST_PP_FILENAME_1       "v8pp/call_v8.hpp"
#       include BOOST_PP_ITERATE()
#   endif // V8PP_CALL_V8_HPP_INCLUDED

#else // BOOST_PP_IS_ITERATING

#  define n BOOST_PP_ITERATION()
#  define V8PP_CALL_V8_args(z, n, data) BOOST_PP_CAT(T,n) BOOST_PP_CAT(arg,n)
#  define V8PP_CALL_V8_tov8(z, n, data) to_v8(BOOST_PP_CAT(arg,n))

namespace v8pp {

template <BOOST_PP_ENUM_PARAMS(n, typename T)>
inline v8::Handle<v8::Value> call_v8(v8::Handle<v8::Function> func, v8::Handle<v8::Object> recv, BOOST_PP_ENUM(n, V8PP_CALL_V8_args, ~))
{
	v8::Handle<v8::Value> argv[] = { BOOST_PP_ENUM(n, V8PP_CALL_V8_tov8, ~) };
	return func->Call(recv, n, argv);
}

template <BOOST_PP_ENUM_PARAMS(n, typename T)>
inline v8::Handle<v8::Value> call_v8(v8::Handle<v8::Function> func, BOOST_PP_ENUM(n, V8PP_CALL_V8_args, ~))
{
	v8::Handle<v8::Value> argv[] = { BOOST_PP_ENUM(n, V8PP_CALL_V8_tov8, ~) };
	return func->Call(func, n, argv);
}

} // namespace v8pp

#   undef V8PP_CALL_V8_tov8
#   undef V8PP_CALL_V8_args
#   undef n

#endif
