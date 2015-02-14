#include <iosfwd>
#include <string>
#include <sstream>
#include <stdexcept>

#include "v8pp/context.hpp"
#include "v8pp/convert.hpp"

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

template<typename T>
T run_script(v8pp::context& context, std::string const& source)
{
	v8::Isolate* isolate = context.isolate();

	v8::HandleScope scope(isolate);
	v8::TryCatch try_catch;
	v8::Handle<v8::Value> result = context.run_script(source);
	if (try_catch.HasCaught())
	{
		std::string const msg = v8pp::from_v8<std::string>(isolate, try_catch.Exception()->ToString());
		throw std::runtime_error(msg);
	}
	return v8pp::from_v8<T>(isolate, result);
}
