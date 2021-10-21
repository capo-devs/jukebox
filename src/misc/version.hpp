#pragma once
#include <misc/buf_string.hpp>
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

	BufString<64> toString(bool full = false) const noexcept;
};
} // namespace jk
