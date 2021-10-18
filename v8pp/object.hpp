#ifndef V8PP_OBJECT_HPP_INCLUDED
#define V8PP_OBJECT_HPP_INCLUDED

#include <cstring>

#include <v8.h>

#include "v8pp/convert.hpp"

namespace v8pp {

/// Get optional value from V8 object by name.
/// Dot symbols in option name delimits subobjects name.
/// return false if the value doesn't exist in the options object
template<typename T>
bool get_option(v8::Isolate* isolate, v8::Local<v8::Object> options,
	string_view name, T& value)
{
	string_view::size_type const dot_pos = name.find('.');
	if (dot_pos != name.npos)
	{
		v8::Local<v8::Object> suboptions;
		return get_option(isolate, options, name.substr(0, dot_pos), suboptions)
			&& get_option(isolate, suboptions, name.substr(dot_pos + 1), value);
	}
	v8::Local<v8::Value> val;
	if (!options->Get(isolate->GetCurrentContext(), v8pp::to_v8(isolate, name)).ToLocal(&val)
		|| val->IsUndefined())
	{
		return false;
	}
	value = from_v8<T>(isolate, val);
	return true;
}

/// Set named value in V8 object
/// Dot symbols in option name delimits subobjects name.
/// return false if the value doesn't exists in the options subobject
template<typename T>
bool set_option(v8::Isolate* isolate, v8::Local<v8::Object> options,
	string_view name, T const& value)
{
	string_view::size_type const dot_pos = name.find('.');
	if (dot_pos != name.npos)
	{
		v8::Local<v8::Object> suboptions;
		return get_option(isolate, options, name.substr(0, dot_pos), suboptions)
			&& set_option(isolate, suboptions, name.substr(dot_pos + 1), value);
	}
	return options->Set(isolate->GetCurrentContext(), v8pp::to_v8(isolate, name), to_v8(isolate, value)).FromJust();
}

/// Set named constant in V8 object
/// Subobject names are not supported
template<typename T>
void set_const(v8::Isolate* isolate, v8::Local<v8::Object> options,
	string_view name, T const& value)
{
	options->DefineOwnProperty(isolate->GetCurrentContext(),
		v8pp::to_v8(isolate, name), to_v8(isolate, value),
		v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete)).FromJust();
}

} // namespace v8pp

#endif // V8PP_OBJECT_HPP_INCLUDED
