#ifndef BOOST_PP_IS_ITERATING
#   ifndef V8PP_CALL_FROM_V8_HPP_INCLUDED
#   define V8PP_CALL_FROM_V8_HPP_INCLUDED
#       include <boost/preprocessor/repetition.hpp>
#       include <boost/preprocessor/iteration/iterate.hpp>

#       include <boost/mpl/vector.hpp>
#       include <boost/mpl/at.hpp>
#       include <boost/mpl/size.hpp>

#       include <v8.h>

#       include "v8pp/config.hpp"
#       include "v8pp/from_v8.hpp"

#       ifndef V8PP_CALL_FROM_V8_MAX_SIZE
#         define V8PP_CALL_FROM_V8_MAX_SIZE V8PP_MAX_ARG_LIMIT
#       endif

namespace v8pp {

namespace detail {

template <typename P, size_t N>
struct call_from_v8_helper_function;

template <typename P, typename T, size_t N>
struct call_from_v8_helper_mem_function;

} // namespace detail

template<typename P>
typename P::return_type
call_from_v8(typename P::function_type ptr, v8::Arguments const& args)
{
	typedef typename P::arguments args_tl;
	enum { arg_count = boost::mpl::size<args_tl>::value };

	if ( args.Length() != arg_count )
	{
		throw std::runtime_error("argument count does not match function definition");
	}

	return detail::call_from_v8_helper_function<P, arg_count>::exec(ptr, args);
}

template<typename P, typename T>
typename P::return_type
call_from_v8(T& obj, typename P::method_type ptr, v8::Arguments const& args)
{
	typedef typename P::arguments args_tl;
	enum { arg_count = boost::mpl::size<args_tl>::value };

	if ( args.Length() != arg_count )
	{
		throw std::runtime_error("argument count does not match function definition");
	}

	return detail::call_from_v8_helper_mem_function<P, T, arg_count>::exec(obj, ptr, args);
}

} // namespace v8pp

#       define BOOST_PP_ITERATION_LIMITS (0, V8PP_CALL_FROM_V8_MAX_SIZE - 1)
#       define BOOST_PP_FILENAME_1       "v8pp/call_from_v8.hpp"
#       include BOOST_PP_ITERATE()
#   endif // V8PP_CALL_FROM_V8_HPP_INCLUDED

#else // BOOST_PP_IS_ITERATING

#  define n BOOST_PP_ITERATION()
#  define V8PP_CALL_V8_from_v8_args(z, n, data) v8pp::from_v8<typename boost::mpl::at_c<args_tl, n>::type>(args[ n ])

namespace v8pp { namespace detail {

template<typename P>
struct call_from_v8_helper_function<P, n>
{
	static typename P::return_type
	exec(typename P::function_type ptr, v8::Arguments const& args)
	{
		typedef typename P::arguments args_tl;
		(void)args;
		return (*ptr)(BOOST_PP_ENUM(n, V8PP_CALL_V8_from_v8_args, ~));
	}
};

template<typename P, typename T>
struct call_from_v8_helper_mem_function<P, T, n>
{
	static typename P::return_type
	exec(T& obj, typename P::method_type ptr, v8::Arguments const& args)
	{
		typedef typename P::arguments args_tl;
		(void)args;
		return (obj.*ptr)(BOOST_PP_ENUM(n, V8PP_CALL_V8_from_v8_args, ~));
	}
};

}} // namespace v8pp::detail

#   undef V8PP_CALL_V8_from_v8_args
#   undef n

#endif
