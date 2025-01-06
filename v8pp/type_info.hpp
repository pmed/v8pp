#pragma once

#include <string_view>

namespace v8pp::detail {

/// Type information for custom RTTI
class type_info
{
public:
	constexpr std::string_view name() const { return name_; }
	constexpr bool operator==(type_info const& other) const { return name_ == other.name_; }
	constexpr bool operator!=(type_info const& other) const { return name_ != other.name_; }

private:
	template<typename T>
	constexpr friend type_info type_id();

	constexpr explicit type_info(std::string_view name)
		: name_(name)
	{
	}

	std::string_view name_;
};

/// Get type information for type T
/// The idea is borrowed from https://github.com/Manu343726/ctti
template<typename T>
constexpr type_info type_id()
{
#if defined(_MSC_VER) && !defined(__clang__)
	std::string_view name = __FUNCSIG__;
	const std::initializer_list<std::string_view> all_prefixes{ "type_id<", "struct ", "class " };
	const std::initializer_list<std::string_view> any_suffixes{ ">" };
#elif defined(__clang__) || defined(__GNUC__)
	std::string_view name = __PRETTY_FUNCTION__;
	const std::initializer_list<std::string_view> all_prefixes{ "T = " };
	const std::initializer_list<std::string_view> any_suffixes{ ";", "]" };
#else
#error "Unknown compiler"
#endif
	for (auto&& prefix : all_prefixes)
	{
		const auto p = name.find(prefix);
		if (p != name.npos)
		{
			name.remove_prefix(p + prefix.size());
		}
	}

	for (auto&& suffix : any_suffixes)
	{
		const auto p = name.rfind(suffix);
		if (p != name.npos)
		{
			name.remove_suffix(name.size() - p);
			break;
		}
	}

	return type_info(name);
}

} // namespace v8pp::detail
