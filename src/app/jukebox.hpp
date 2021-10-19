#pragma once
#include <app/player.hpp>
#include <ktl/fixed_vector.hpp>
#include <misc/delegate.hpp>
#include <misc/delta_time.hpp>
#include <win/key.hpp>
#include <memory>
#include <optional>

struct GLFWwindow;

namespace jk {
class GlfwInstance;

struct SliderFloat {
	std::optional<float> select;

	std::optional<float> operator()(char const* name, float value, float min = 0.0f, float max = 0.0f, char const* label = "");
};

class Jukebox {
  public:
	enum class Status { eRun, eQuit };
	using OnKey = Delegate<Key>::Signal;
	using OnFileDrop = Delegate<std::span<str_t const>>::Signal;

	static std::unique_ptr<Jukebox> make(GlfwInstance& instance, ktl::not_null<GLFWwindow*> window);

	Jukebox(Jukebox&&) = delete;
	Jukebox& operator=(Jukebox&&) = delete;

	Status tick(Time dt);

  private:
	Jukebox(GlfwInstance& instance, ktl::not_null<GLFWwindow*> window);

	capo::Instance m_capo;
	ktl::fixed_vector<Key, 16> m_keys;
	Player m_player;
	OnKey m_onKey;
	OnFileDrop m_onFileDrop;
	SliderFloat m_seek;
	ktl::not_null<GLFWwindow*> m_window;
	bool m_showImguiDemo{};
};
} // namespace jk
