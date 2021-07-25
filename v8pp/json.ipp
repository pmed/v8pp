#include "v8pp/json.hpp"

namespace v8pp {

V8PP_IMPL std::string json_str(v8::Isolate* isolate, v8::Local<v8::Value> value)
{
	if (value.IsEmpty())
	{
		return std::string();
	}

	v8::HandleScope scope(isolate);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::String> result = v8::JSON::Stringify(context, value).ToLocalChecked();
	v8::String::Utf8Value const str(isolate, result);

	return std::string(*str, str.length());
}

V8PP_IMPL v8::Local<v8::Value> json_parse(v8::Isolate* isolate, string_view str)
{
	if (str.empty())
	{
		return v8::Local<v8::Value>();
	}

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::String> value = v8::String::NewFromUtf8(isolate, str.data(),
		v8::NewStringType::kNormal, static_cast<int>(str.size())).ToLocalChecked();

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Value> result;
	bool const is_empty_result = v8::JSON::Parse(context, value).ToLocal(&result);
	(void)is_empty_result;
	if (try_catch.HasCaught())
	{
		result = try_catch.Exception();
	}
	return scope.Escape(result);
}

V8PP_IMPL v8::Local<v8::Object> json_object(v8::Isolate* isolate, v8::Local<v8::Object> object, bool with_functions)
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> result = v8::Object::New(isolate);
	v8::Local<v8::Array> prop_names = object->GetPropertyNames(context).ToLocalChecked();
	for (uint32_t i = 0, count = prop_names->Length(); i < count; ++i)
	{
		v8::Local<v8::Value> name, value;
		if (prop_names->Get(context, i).ToLocal(&name)
			&& object->Get(context, name).ToLocal(&value))
		{
			if (value->IsFunction())
			{
				if (!with_functions) continue;
				value = value.As<v8::Function>()->ToString(context).FromMaybe(v8::String::Empty(isolate));
			}
			else if (value->IsObject() || value->IsArray())
			{
				value = v8::JSON::Stringify(context, value).FromMaybe(v8::String::Empty(isolate));
			}
			const bool r = result->Set(context, name, value).ToChecked();
			(void)r;
		}
	}
	return scope.Escape(result);
}

} // namespace v8pp
