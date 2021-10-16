#pragma once
#include <time.h>
#include <chrono>
#include <string>
#include <utility>

using namespace std::chrono_literals;
namespace stdch = std::chrono;

namespace jk {
using Clock = stdch::steady_clock;
using Time = stdch::duration<float>;

template <typename TClock = stdch::system_clock>
std::string timeStr(std::string_view fmt = "%T", typename TClock::time_point const& stamp = TClock::now());

struct DeltaTime {
	Clock::time_point stamp = Clock::now();
	Time value{};

	constexpr operator Time() const noexcept { return value; }

	Time operator++() noexcept {
		auto const t = Clock::now();
		value = t - std::exchange(stamp, t);
		return value;
	}
};

// impl

template <typename TClock>
std::string timeStr(std::string_view fmt, typename TClock::time_point const& stamp) {
	std::time_t const stamp_time = TClock::to_time_t(stamp);
	std::tm const stamp_tm = *std::localtime(&stamp_time);
	char ret[1024];
	std::strftime(ret, 1024, fmt.data(), &stamp_tm);
	return ret;
}
} // namespace jk
