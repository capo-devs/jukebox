#pragma once
#include <cstdio>
#include <string_view>

namespace jk {
template <std::size_t N>
class BufString {
  public:
	constexpr BufString() = default;

	template <typename Arg, typename... Args>
	BufString(char const* fmt, Arg arg, Args... args) noexcept {
		std::snprintf(m_buf, N, fmt, arg, args...);
	}

	template <std::size_t M>
	constexpr BufString(char const (&str)[M]) noexcept : BufString(str, M) {}

	constexpr BufString(char const* begin, std::size_t extent) noexcept {
		for (std::size_t i = 0; i < extent && i + 1 < N; ++i) { m_buf[i] = *begin++; }
	}

	constexpr BufString(std::string_view str) noexcept : BufString(str.data(), str.size()) {}

	constexpr std::string_view get() const noexcept { return m_buf; }
	constexpr char const* data() const noexcept { return m_buf; }
	constexpr operator std::string_view() const noexcept { return get(); }

  private:
	char m_buf[N] = {};
};
} // namespace jk
