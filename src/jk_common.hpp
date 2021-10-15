#pragma once

constexpr bool jk_debug =
#if defined(JK_DEBUG)
	true;
#else
	false;
#endif

namespace jk {
template <typename... T>
constexpr bool allValid(T... t) noexcept {
	return ((t != T{}) && ...);
}

template <typename T, typename... Ts>
constexpr bool anyOf(T t, Ts... ts) noexcept {
	return ((t == ts) || ...);
}
} // namespace jk
