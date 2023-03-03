#pragma once

#include "v8pp/call_from_v8.hpp"
#include "v8pp/context.hpp"
#include "v8pp/function.hpp"
#include "v8pp/ptr_traits.hpp"

#include "v8pp/call_v8.hpp"
#include "v8pp/module.hpp"
#include "v8pp/class.hpp"

#include <functional>

static bool is_valid_v8array(v8::Isolate*, const v8::Local<v8::Value>& value)
{
	return !value.IsEmpty() && value->IsArray();
}

// static bool is_valid_v8set(v8::Isolate*, const v8::Local<v8::Value>& value)
//{
//	return !value.IsEmpty() && value->IsSet();
// }

static bool is_valid(v8::Isolate*, v8::Local<v8::Value> value)
{
	return !value.IsEmpty() && value->IsObject() && !value->IsArray();
}

template<class T>
static bool loop_v8_array_of_val(v8::Isolate* isolate, const v8::Local<v8::Value>& value, std::function<bool(const T&)> func)
{
	if (!is_valid_v8array(isolate, value)) return false;

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Array> elements = value.As<v8::Array>();
	for (uint32_t i = 0, count = elements->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> item = elements->Get(context, i).ToLocalChecked();
		auto val = v8pp::convert<T>::from_v8(isolate, item);
		if (!func(val)) break;
	}
	return true;
}

// set currently not supported. use Map<xxx, bool> instead, and set value to true.
// template<class T>
// static bool loop_v8_set_of_val(v8::Isolate* isolate, const v8::Local<v8::Value>& value, std::function<bool(const T&)> func)
//{
//	if (!is_valid_v8set(isolate, value)) return false;
//
//	v8::HandleScope scope(isolate);
//	v8::Local<v8::Context> context = isolate->GetCurrentContext();
//	v8::Local<v8::Set> elements = value.As<v8::Set>();
//	for (uint32_t i = 0, count = (uint32_t)elements->Size(); i < count; ++i)
//	{
//		v8::Local<v8::Value> item = elements->Get(context, i).ToLocalChecked();
//		auto val = v8pp::convert<T>::from_v8(isolate, item);
//		if (!func(val)) break;
//	}
//	return true;
//}

template<class K, class V>
static bool loop_v8_map_of_val(v8::Isolate* isolate, const v8::Local<v8::Value>& value, std::function<bool(const K&, const V&)> func)
{
	if (!is_valid(isolate, value)) return false;

	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = value.As<v8::Object>();
	v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();

	for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> key = prop_names->Get(context, i).ToLocalChecked();
		v8::Local<v8::Value> val = object->Get(context, key).ToLocalChecked();
		const auto k = v8pp::convert<K>::from_v8(isolate, key);
		const auto v = v8pp::convert<V>::from_v8(isolate, val);
		if (!func(k, v)) break;
	}

	return true;
}
