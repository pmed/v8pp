//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef V8PP_PTR_TRAITS_HPP_INCLUDED
#define V8PP_PTR_TRAITS_HPP_INCLUDED

#include <memory>

namespace v8pp {

template<typename T, typename Enable = void>
struct convert;

struct raw_ptr_traits
{
	using pointer_type = void*;
	using const_pointer_type = void const*;

	template<typename T>
	using object_pointer_type = T*;
	template<typename T>
	using object_const_pointer_type = T const*;

	using object_id = void*;

	static object_id pointer_id(void* ptr) { return ptr; }
	static pointer_type key(object_id id) { return id; }
	static pointer_type const_pointer_cast(const_pointer_type ptr) { return const_cast<void*>(ptr); }
	template<typename T, typename U>
	static T* static_pointer_cast(U* ptr) { return static_cast<T*>(ptr); }

	template<typename T>
	using convert_ptr = convert<T*>;

	template<typename T>
	using convert_ref = convert<T&>;

	template<typename T, typename ...Args>
	static object_pointer_type<T> create(Args&&... args)
	{
		return new T(std::forward<Args>(args)...);
	}

	template<typename T>
	static void destroy(object_pointer_type<T> const& object)
	{
		delete object;
	}

	template<typename T>
	static size_t object_size(object_pointer_type<T> const&)
	{
		return sizeof(T);
	}
};

struct ref_from_shared_ptr {};

struct shared_ptr_traits
{
	using pointer_type = std::shared_ptr<void>;
	using const_pointer_type = std::shared_ptr<void const>;

	template<typename T>
	using object_pointer_type = std::shared_ptr<T>;
	template<typename T>
	using object_const_pointer_type = std::shared_ptr<T const>;

	using object_id = void*;

	static object_id pointer_id(pointer_type const& ptr) { return ptr.get(); }
	static pointer_type key(object_id id) { return std::shared_ptr<void>(id, [](void*) {}); }
	static pointer_type const_pointer_cast(const_pointer_type const& ptr) { return std::const_pointer_cast<void>(ptr); }
	template<typename T, typename U>
	static std::shared_ptr<T> static_pointer_cast(std::shared_ptr<U> const& ptr) { return std::static_pointer_cast<T>(ptr); }

	template<typename T>
	using convert_ptr = convert<std::shared_ptr<T>>;

	template<typename T>
	using convert_ref = convert<T, ref_from_shared_ptr>;

	template<typename T, typename ...Args>
	static object_pointer_type<T> create(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	static void destroy(object_pointer_type<T> const&)
	{
		// do nothing with reference-counted object
	}

	template<typename T>
	static size_t object_size(object_pointer_type<T> const&)
	{
		return sizeof(T);
	}
};

} //namespace v8pp

#endif // V8PP_PTR_TRAITS_HPP_INCLUDED
