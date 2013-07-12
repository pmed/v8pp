#ifndef BOOST_PP_IS_ITERATING
#   ifndef V8PP_DETAIL_PROTO_HPP_INCLUDED
#   define V8PP_DETAIL_PROTO_HPP_INCLUDED
#       include <boost/preprocessor/repetition.hpp>
#       include <boost/preprocessor/punctuation/comma_if.hpp>
#       include <boost/preprocessor/iteration/iterate.hpp>

#       include <boost/mpl/vector.hpp>
#       include <boost/mpl/and.hpp>

#       include <boost/type_traits/is_function.hpp>
#       include <boost/type_traits/is_member_function_pointer.hpp>
#       include <boost/type_traits/is_pointer.hpp>
#       include <boost/type_traits/remove_pointer.hpp>

#       include <boost/utility/enable_if.hpp>

#       include "config.hpp"
#       ifndef V8PP_PROTO_MAX_SIZE
#         define V8PP_PROTO_MAX_SIZE V8PP_MAX_ARG_LIMIT
#       endif
namespace v8pp { namespace detail {

template<typename T>
struct function_ptr;

template<typename C, typename T>
struct mem_function_ptr;

template<typename T>
struct is_function_pointer : boost::mpl::and_<
		boost::is_pointer<T>,
		boost::is_function<typename boost::remove_pointer<T>::type>
	>
{
};

template<typename P>
struct is_void_return : boost::is_same<void, typename P::return_type>::type
{
};

}} // namespace v8pp::detail
#       define BOOST_PP_ITERATION_LIMITS (0, V8PP_PROTO_MAX_SIZE - 1)
#       define BOOST_PP_FILENAME_1       "proto.hpp"
#       include BOOST_PP_ITERATE()
#    endif // V8PP_DETAIL_FROM_V8_ARGUMENTS_HPP_INCLUDED

#else // BOOST_PP_IS_ITERATING

#   define n BOOST_PP_ITERATION()

namespace v8pp { namespace detail {

template<typename R BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, typename A)>
struct function_ptr<R (*)( BOOST_PP_ENUM_PARAMS(n, A) )>
{
	typedef R return_type;
	typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n, A)> arguments;
	typedef R (*function_type)(BOOST_PP_ENUM_PARAMS(n, A));
};

template<typename C, typename R BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, typename A)>
struct mem_function_ptr<C, R (C::*)( BOOST_PP_ENUM_PARAMS(n, A) )>
{
	typedef R return_type;
	typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n, A)> arguments;
	typedef R (C::*method_type)(BOOST_PP_ENUM_PARAMS(n, A));
};

template<typename C, typename R BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, typename A)>
struct mem_function_ptr<C, R (C::*)( BOOST_PP_ENUM_PARAMS(n, A) ) const>
{
	typedef R return_type;
	typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n, A)> arguments;
	typedef R (C::*method_type)(BOOST_PP_ENUM_PARAMS(n, A)) const;
};

}} // namespace v8pp::detail

#   undef n

#endif // BOOST_PP_IS_ITERATING
