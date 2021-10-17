#pragma once
#include <cstdint>
#include <type_traits>

namespace jk {
///
/// \brief 2D vector
///
template <typename T>
	requires std::is_arithmetic_v<T>
struct TVec2 {
	T x, y;
};

using UVec2 = TVec2<std::uint32_t>;
} // namespace jk
