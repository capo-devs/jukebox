#pragma once
#include <chrono>
#include <utility>

using namespace std::chrono_literals;
namespace stdch = std::chrono;

namespace jk {
using Clock = stdch::steady_clock;
using Time = stdch::duration<float>;

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
} // namespace jk
