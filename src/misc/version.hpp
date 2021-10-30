#pragma once
#include <ktl/stack_string.hpp>
#include <compare>

namespace jk {
struct Version {
	int major{};
	int minor{};
	int patch{};
	int tweak{};

	static Version parse(std::string_view str) noexcept;
	static Version app() noexcept;

	constexpr auto operator<=>(Version const&) const = default;
	constexpr bool compatible(Version const& target) const noexcept { return target <= *this; }

	ktl::stack_string<64> toString(bool full = false) const noexcept;
};
} // namespace jk
