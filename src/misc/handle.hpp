#pragma once
#include <cstdint>
#include <type_traits>

namespace jk {
///
/// \brief RAII integral handle
///
template <typename T = std::uint32_t>
	requires std::is_integral_v<T>
struct THandle {
	using type = T;

	T value;

	constexpr THandle(T t = T{}) noexcept : value(t) {}
	constexpr THandle(THandle const& rhs) noexcept : THandle(rhs.value) {}
	constexpr THandle(THandle&& rhs) noexcept : THandle() { std::swap(value, rhs.value); }
	constexpr THandle& operator=(THandle rhs) noexcept { return (std::swap(value, rhs.value), *this); }

	constexpr operator T const() noexcept { return value; }
	constexpr bool operator==(THandle const&) const = default;
};

constexpr auto null_handle_v = THandle{};
} // namespace jk
