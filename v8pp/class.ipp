#include "v8pp/class.hpp"

#include <cstdio>

namespace v8pp {

namespace detail {

V8PP_IMPL std::string pointer_str(void const* ptr)
{
	std::string buf(sizeof(void*) * 2 + 3, 0); // +3 for 0x and \0 terminator
	int const len =
#if defined(_MSC_VER) && (_MSC_VER < 1900)
		sprintf_s(&buf[0], buf.size(), "%p", ptr);
#else
		snprintf(&buf[0], buf.size(), "%p", ptr);
#endif
	buf.resize(len < 0 ? 0 : len);
	return buf;
}

V8PP_IMPL class_info::class_info(type_info const& type, type_info const& traits)
	: type(type)
	, traits(traits)
{
}

V8PP_IMPL std::string class_info::class_name() const
{
	return "v8pp::class_<" + type.name().str() + ", " + traits.name().str() + ">";
}

V8PP_IMPL void classes::remove_all(v8::Isolate* isolate)
{
	instance(operation::remove, isolate);
}

V8PP_IMPL classes::classes_info::iterator classes::find(type_info const& type)
{
	return std::find_if(classes_.begin(), classes_.end(),
		[&type](classes_info::value_type const& info)
	{
		return info->type == type;
	});
}

V8PP_IMPL classes* classes::instance(operation op, v8::Isolate* isolate)
{
#if defined(V8PP_ISOLATE_DATA_SLOT)
	classes* info = static_cast<classes*>(
		isolate->GetData(V8PP_ISOLATE_DATA_SLOT));
	switch (op)
	{
	case operation::get:
		return info;
	case operation::add:
		if (!info)
		{
			info = new classes;
			isolate->SetData(V8PP_ISOLATE_DATA_SLOT, info);
		}
		return info;
	case operation::remove:
		if (info)
		{
			delete info;
			isolate->SetData(V8PP_ISOLATE_DATA_SLOT, nullptr);
		}
	default:
		return nullptr;
	}
#else
	static std::unordered_map<v8::Isolate*, classes> instances;
	switch (op)
	{
	case operation::get:
	{
		auto it = instances.find(isolate);
		return it != instances.end() ? &it->second : nullptr;
	}
	case operation::add:
		return &instances[isolate];
	case operation::remove:
		instances.erase(isolate);
	default:
		return nullptr;
	}
#endif
}

} // namespace detail

V8PP_IMPL void cleanup(v8::Isolate* isolate)
{
	detail::classes::remove_all(isolate);
}

} // namespace v8pp
