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

struct LazySliderFloat {
	std::optional<float> prev;

	std::optional<float> operator()(char const* name, float value, float min = 0.0f, float max = 0.0f, char const* label = "");
};

class FileBrowser {
  public:
	inline static constexpr std::string_view title_v = "Click to Add";

	FileBrowser();
	FileBrowser(FileBrowser&&) noexcept;
	FileBrowser& operator=(FileBrowser&&) noexcept;
	~FileBrowser() noexcept;

	std::string operator()();

	bool m_show{};

  private:
	struct Impl;

	std::unique_ptr<Impl> m_impl;
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

	void mainControls();
	void seekBar();
	void trackControls();
	void tracklist();

	capo::Instance m_capo;
	char m_savePath[256] = "jukebox_playlist.txt";
	ktl::fixed_vector<Key, 16> m_keys;
	Player m_player;
	OnKey m_onKey;
	OnFileDrop m_onFileDrop;
	LazySliderFloat m_seek;
	FileBrowser m_browser;
	ktl::not_null<GLFWwindow*> m_window;
	bool m_saveFailure{};
	bool m_showImguiDemo{};
};
} // namespace jk
