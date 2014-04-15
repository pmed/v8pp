#ifndef BOOST_PP_IS_ITERATING
#   ifndef V8PP_FACTORY_HPP_INCLUDED
#   define V8PP_FACTORY_HPP_INCLUDED
#       include "config.hpp"

#       include <boost/preprocessor/repetition.hpp>
#       include <boost/preprocessor/arithmetic/sub.hpp>
#       include <boost/preprocessor/punctuation/comma_if.hpp>
#       include <boost/preprocessor/iteration/iterate.hpp>

#       include <boost/mpl/vector.hpp>
#        include <boost/mpl/void.hpp>

#       include <v8.h>
#       include "v8pp/config.hpp"

#       ifndef V8PP_FACTORY_MAX_SIZE
#         define V8PP_FACTORY_MAX_SIZE V8PP_MAX_ARG_LIMIT
#       endif

namespace v8pp {

// Factory that prevents class from being constructed in JavaScript
struct no_factory
{
	template<typename C>
	struct instance
	{
		static void destroy(C* object)
		{
			v8::V8::AdjustAmountOfExternalAllocatedMemory(-static_cast<intptr_t>(sizeof(C)));
			delete object;
		}
	};
};

// Factory that calls C++ constructor with v8::Arguments directly
struct v8_args_factory
{
	template<typename C>
	struct instance : no_factory::instance<C>
	{
		static C* create(v8::Arguments const& args)
		{
			v8::V8::AdjustAmountOfExternalAllocatedMemory(static_cast<intptr_t>(sizeof(C)));
			return new C(args);
		}
	};
};

// Primary factory template
template<BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(V8PP_FACTORY_MAX_SIZE, typename T, boost::mpl::void_)>
struct factory;

// Enum types for no_factory and v8_args_factory tagging in class_ constructor
enum no_ctor_t { no_ctor };
enum v8_args_ctor_t { v8_args_ctor };

// Template function to specifiy factory arguments in class_ constructor
template<BOOST_PP_ENUM_PARAMS_Z(1, V8PP_FACTORY_MAX_SIZE, typename T)>
factory<BOOST_PP_ENUM_PARAMS_Z(1, V8PP_FACTORY_MAX_SIZE, T)> ctor();

} //namespace v8pp

#       define BOOST_PP_ITERATION_LIMITS (0, V8PP_FACTORY_MAX_SIZE - 1)
#       define BOOST_PP_FILENAME_1       "v8pp/factory.hpp"
#       include BOOST_PP_ITERATE()
#   endif // V8PP_FACTORY_HPP_INCLUDED

#else // BOOST_PP_IS_ITERATING

#  define n BOOST_PP_ITERATION()
#  define V8PP_FACTORY_args(z, n, data) BOOST_PP_CAT(T,n) BOOST_PP_CAT(arg,n)

namespace v8pp {

// specialization pattern
template<BOOST_PP_ENUM_PARAMS(n, typename T)>
struct factory<BOOST_PP_ENUM_PARAMS(n, T)>
{
	template<typename C>
	struct instance: no_factory::instance<C>
	{
		typedef C type;

		// mimics to function_ptr<> to allow usage with with call_from_v8()
		typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n,T)> arguments;
		typedef C* return_type;
		typedef C* (*function_type)(BOOST_PP_ENUM(n, V8PP_FACTORY_args, ~));

		static C* create(BOOST_PP_ENUM(n, V8PP_FACTORY_args, ~))
		{
			v8::V8::AdjustAmountOfExternalAllocatedMemory(static_cast<intptr_t>(sizeof(C)));
			return new C(BOOST_PP_ENUM_PARAMS(n,arg));
		}
	};
};

template<BOOST_PP_ENUM_PARAMS(n, typename T)>
inline factory<BOOST_PP_ENUM_PARAMS(n, T)>
ctor() { return factory<BOOST_PP_ENUM_PARAMS(n, T)>(); }

} // namespace v8pp

#   undef V8PP_FACTORY_args
#   undef n

#endif
