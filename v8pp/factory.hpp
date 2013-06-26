#ifndef BOOST_PP_IS_ITERATING
#   ifndef V8PP_FACTORY_HPP_INCLUDED
#   define V8PP_FACTORY_HPP_INCLUDED
#       include "config.hpp"

#       include <boost/preprocessor/repetition.hpp>
#       include <boost/preprocessor/arithmetic/sub.hpp>
#       include <boost/preprocessor/punctuation/comma_if.hpp>
#       include <boost/preprocessor/iteration/iterate.hpp>

#       include <boost/mpl/vector.hpp>

#       include <v8.h>


#       ifndef V8PP_FACTORY_MAX_SIZE
#         define V8PP_FACTORY_MAX_SIZE V8PP_MAX_ARG_LIMIT
#       endif

#       define V8PP_FACTORY_header(z, n, data) typename BOOST_PP_CAT(T,n) = none

namespace v8pp {

struct none {};

// Factory that calls C++ constructor with v8::Arguments directly
struct v8_args_factory {};

// Factory that prevents class from being constructed in JavaScript
struct no_factory {};

// primary template
template<BOOST_PP_ENUM(V8PP_FACTORY_MAX_SIZE, V8PP_FACTORY_header, ~)>
struct factory;

} //namespace v8pp

#       define BOOST_PP_ITERATION_LIMITS (0, V8PP_FACTORY_MAX_SIZE - 1)
#       define BOOST_PP_FILENAME_1       "factory.hpp"
#       include BOOST_PP_ITERATE()
#   endif // V8PP_FACTORY_HPP_INCLUDED

#else // BOOST_PP_IS_ITERATING

#  define n BOOST_PP_ITERATION()
#  define V8PP_FACTORY_default(z, n, data) data
#  define V8PP_FACTORY_args(z, n, data) BOOST_PP_CAT(T,n) BOOST_PP_CAT(arg,n)

namespace v8pp {

// specialization pattern
template<BOOST_PP_ENUM_PARAMS(n, typename T)>
struct factory<
	BOOST_PP_ENUM_PARAMS(n,T)
	BOOST_PP_COMMA_IF(n)
	BOOST_PP_ENUM(BOOST_PP_SUB(V8PP_FACTORY_MAX_SIZE,n), V8PP_FACTORY_default, none)
	>
{
	template<typename C>
	struct construct
	{
		typedef C type;

		// mimics to function_ptr<> to allow usage with with call_from_v8()
		typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n,T)> arguments;
		typedef C* return_type;
		typedef C* (*function_type)(BOOST_PP_ENUM(n, V8PP_FACTORY_args, ~));

		static C* create(BOOST_PP_ENUM(n, V8PP_FACTORY_args, ~))
		{
			return new C(BOOST_PP_ENUM_PARAMS(n,arg));
		}
	};
};

} // namespace v8pp

#   undef V8PP_FACTORY_default
#   undef V8PP_FACTORY_args
#   undef n

#endif
