#include "v8pp/json.hpp"
#include "v8pp/convert.hpp"

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
	return v8pp::from_v8<std::string>(isolate, result);
}

V8PP_IMPL v8::Local<v8::Value> json_parse(v8::Isolate* isolate, std::string_view str)
{
	if (str.empty())
	{
		return v8::Local<v8::Value>();
	}

	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	v8::TryCatch try_catch(isolate);
	v8::Local<v8::Value> result;
	bool const is_empty_result = v8::JSON::Parse(context, v8pp::to_v8(isolate, str)).ToLocal(&result);
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
			result->Set(context, name, value).FromJust();
		}
	}
	return scope.Escape(result);
}

} // namespace v8pp
