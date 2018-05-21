//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <iosfwd>
#include <array>
#include <string>
#include <sstream>
#include <stdexcept>

#include "v8pp/context.hpp"
#include "v8pp/convert.hpp"
#include "v8pp/utility.hpp"

template<typename Char, typename Traits,
	typename T, typename Alloc, typename ...Other,
	template<typename, typename, typename ...> class Sequence>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os,
	Sequence<T, Alloc, Other...> const& sequence)
{
	os << '[';
	bool first = true;
	for (auto const& item : sequence)
	{
		if (!first) os << ", ";
		os << item;
		first = false;
	}
	os << ']';
	return os;
}

template<typename Char, typename Traits, typename T, size_t N>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os,
	std::array<T, N> const& array)
{
	os << '[';
	bool first = true;
	for (auto const& item : array)
	{
		if (!first) os << ", ";
		os << item;
		first = false;
	}
	os << ']';
	return os;
}

template<typename Char, typename Traits,
	typename Key, typename Value, typename Less, typename Alloc, typename ...Other,
	template<typename, typename, typename, typename, typename ...> class Mapping>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os,
	Mapping<Key, Value, Less, Alloc, Other...> const& mapping)
{
	os << '{';
	bool first = true;
	for (auto const& item : mapping)
	{
		if (!first) os << ", ";
		os << item.first << ": " <<item.second;
		first = false;
	}
	os << '}';
	return os;
}

template<typename Char, typename Traits,
	typename Enum, typename = typename std::enable_if<std::is_enum<Enum>::value>::type>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os, Enum value)
{
	return os << static_cast<typename std::underlying_type<Enum>::type>(value);
}

inline void check(std::string msg, bool condition)
{
	if (!condition)
	{
		std::stringstream ss;
		ss << "check failed: " << msg;
		throw std::runtime_error(ss.str());
	}
}

template<typename T, typename U>
void check_eq(std::string msg, T actual, U expected)
{
	if (actual != expected)
	{
		std::stringstream ss;
		ss << msg << " expected: '" << expected << "' actual: '" << actual << "'";
		check(ss.str(), false);
	}
}

template<typename Ex, typename F>
void check_ex(std::string msg, F&& f)
{
	try
	{
		f();
		check(msg + " expected " + v8pp::detail::type_id<Ex>().name() + " exception", false);
	}
	catch (Ex const&)
	{
	}
}

template<typename T>
T run_script(v8pp::context& context, std::string const& source)
{
	v8::Isolate* isolate = context.isolate();

	v8::HandleScope scope(isolate);
	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Value> result = context.run_script(source);
	if (try_catch.HasCaught())
	{
		std::string const msg = v8pp::from_v8<std::string>(isolate,
			try_catch.Exception()->ToString(isolate->GetCurrentContext()).ToLocalChecked());
		throw std::runtime_error(msg);
	}
	return v8pp::from_v8<T>(isolate, result);
}
