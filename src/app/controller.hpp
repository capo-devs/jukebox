#pragma once
#include <ktl/delegate.hpp>
#include <ktl/fixed_vector.hpp>
#include <win/key.hpp>

namespace jk {
class Controller {
  public:
	enum class Action { eNone, ePlayPause, eStop, eMute, eNext, ePrev, eSeek, eVolume, eQuit };

	struct Response {
		float value{};
		Action action{};
	};
	using ResponseList = ktl::fixed_vector<Response, 4>;

	using Input = ktl::delegate<Key>;

	Controller(Input::signal&& signal);

	ResponseList update() noexcept { return std::exchange(m_list, ResponseList()); }

  private:
	void onKey(Key key) noexcept;
	void add(Action action, float value = {}) noexcept;

	ResponseList m_list;
	Input::signal m_signal;
};
} // namespace jk
