//
// Copyright (c) 2013-2016 Pavel Medvedev. All rights reserved.
//
// This file is part of v8pp (https://github.com/pmed/v8pp) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <iosfwd>

#include <string>
#include <sstream>
#include <stdexcept>
#include <tuple>

// containers
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <utility>

#include "v8pp/context.hpp"
#include "v8pp/convert.hpp"
#include "v8pp/utility.hpp"

template<typename Sequence>
std::ostream& print_sequence(std::ostream& os, Sequence const& sequence, char const brackets[2]);

template<typename T, std::size_t N>
std::ostream& operator<<(std::ostream& os, std::array<T, N> const& array)
{
	return print_sequence(os, array, "[]");
}

template<typename Char, typename Traits, typename Alloc, typename = typename std::enable_if<!std::is_same<Char, char>::value>::type>
std::ostream& operator<<(std::ostream& os, std::basic_string<Char, Traits, Alloc> const& string)
{
	return print_sequence(os, string, "''");
}

template<typename Char, typename Traits, typename = typename std::enable_if_t<!std::is_same_v<Char, char>>>
std::ostream& operator<<(std::ostream& os, std::basic_string_view<Char, Traits> const& string_view)
{
	return print_sequence(os, string_view, "''");
}

template<typename T, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::vector<T, Alloc> const& vector)
{
	return print_sequence(os, vector, "[]");
}

template<typename T, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::list<T, Alloc> const& list)
{
	return print_sequence(os, list, "[]");
}

template<typename T, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::forward_list<T, Alloc> const& fwd_list)
{
	return print_sequence(os, fwd_list, "[]");
}

template<typename T, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::deque<T, Alloc> const& deque)
{
	return print_sequence(os, deque, "[]");
}

template<typename Key, typename Comp, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::set<Key, Comp, Alloc> const& set)
{
	return print_sequence(os, set, "[]");
}

template<typename Key, typename Comp, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::multiset<Key, Comp, Alloc> const& multiset)
{
	return print_sequence(os, multiset, "[]");
}

template<typename Key, typename Value, typename Comp, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::map<Key, Value, Comp, Alloc> const& map)
{
	return print_sequence(os, map, "{}");
}

template<typename Key, typename Value, typename Comp, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::multimap<Key, Value, Comp, Alloc> const& multimap)
{
	return print_sequence(os, multimap, "{}");
}

template<typename Key, typename Hash, typename Eq, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::unordered_set<Key, Hash, Eq, Alloc> const& unordered_set)
{
	return print_sequence(os, unordered_set, "[]");
}

template<typename Key, typename Hash, typename Eq, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::unordered_multiset<Key, Hash, Eq, Alloc> const& unordered_multiset)
{
	return print_sequence(os, unordered_multiset, "[]");
}

template<typename Key, typename T, typename Hash, typename Eq, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::unordered_map<Key, T, Hash, Eq, Alloc> const& unordered_map)
{
	return print_sequence(os, unordered_map, "{}");
}

template<typename Key, typename T, typename Hash, typename Eq, typename Alloc>
std::ostream& operator<<(std::ostream& os, std::unordered_multimap<Key, T, Hash, Eq, Alloc> const& unordered_multimap)
{
	return print_sequence(os, unordered_multimap, "{}");
}

template<typename First, typename Second>
std::ostream& operator<<(std::ostream& os, std::pair<First, Second> const& pair)
{
	return os << pair.first << ": " << pair.second;
}

template<typename Enum, typename = typename std::enable_if<std::is_enum<Enum>::value>::type>
std::ostream& operator<<(std::ostream& os, Enum value)
{
	return os << static_cast<typename std::underlying_type<Enum>::type>(value);
}

template<typename Char, typename Traits, typename... Ts>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& os,
	std::tuple<Ts...> const& tuple)
{
	std::apply([&os](auto&&... elems) mutable
	{
		bool first = true;
		os << '(';
		((os << (first? (first = false, "") : ", ") << elems),...);
		os << ')';
	}, tuple);
	return os;
}

template<typename Sequence>
std::ostream& print_sequence(std::ostream& os, Sequence const& sequence, char const brackets[2])
{
	os << brackets[0];
	bool first = true;
	for (auto const& item : sequence)
	{
		if (!first) os << ", ";
		os << item;
		first = false;
	}
	os << brackets[1];
	return os;
}

inline void check(std::string_view msg, bool condition)
{
	if (!condition)
	{
		std::stringstream ss;
		ss << "check failed: " << msg;
		throw std::runtime_error(ss.str());
	}
}

template<typename T, typename U>
void check_eq(std::string_view msg, T actual, U expected)
{
	if (actual != expected)
	{
		std::stringstream ss;
		ss << msg << " expected: '" << expected << "' actual: '" << actual << "'";
		check(ss.str(), false);
	}
}

template<typename Ex, typename F>
void check_ex(std::string_view msg, F&& f)
{
	try
	{
		f();
		std::stringstream ss;
		ss << msg << " expected " << v8pp::detail::type_id<Ex>().name() << " exception";
		check(ss.str(), false);
	}
	catch (Ex const&)
	{
	}
}

template<typename T>
T run_script(v8pp::context& context, std::string_view source)
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
