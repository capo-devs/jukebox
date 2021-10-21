#pragma once
#include <ktl/fixed_vector.hpp>
#include <misc/delegate.hpp>
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

	using Input = Delegate<Key>;

	Controller(Input::Signal&& signal);

	ResponseList update() noexcept { return std::exchange(m_list, ResponseList()); }

  private:
	void onKey(Key key) noexcept;
	void add(Action action, float value = {}) noexcept;

	ResponseList m_list;
	Input::Signal m_signal;
};
} // namespace jk
