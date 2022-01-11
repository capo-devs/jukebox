#pragma once
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

	void onKey(Key key) noexcept;
	ResponseList responses() noexcept { return std::move(m_list); }

  private:
	void add(Action action, float value = {}) noexcept;
	void replaceBindings() noexcept;

	ResponseList m_list;
};
} // namespace jk
