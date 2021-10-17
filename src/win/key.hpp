#pragma once
#include <GLFW/glfw3.h>

namespace jk {
///
/// \brief GLFW Key
///
struct Key {
	int code{};
	int action{};
	int mods{};

	constexpr bool is(int code) const noexcept { return code == this->code; }
	constexpr bool press() const noexcept { return action == GLFW_PRESS; }
	constexpr bool repeat() const noexcept { return action == GLFW_REPEAT; }
	constexpr bool release() const noexcept { return action == GLFW_RELEASE; }
	constexpr bool any(int mods) const noexcept { return (mods & this->mods) != 0; }
	constexpr bool all(int mods) const noexcept { return (mods & this->mods) == mods; }

	template <typename Container>
	static constexpr bool chord(Container const& keys, int code, int mods) noexcept;
};

// impl

template <typename Container>
constexpr bool Key::chord(Container const& keys, int const code, int const mods) noexcept {
	for (Key const& k : keys) {
		if (code == k.code && k.all(mods)) { return true; }
	}
	return false;
}
} // namespace jk
