#pragma once
#include <app/controller.hpp>
#include <app/player.hpp>
#include <app/props.hpp>
#include <ktl/delegate.hpp>
#include <ktl/fixed_vector.hpp>
#include <misc/delta_time.hpp>
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
	using OnKey = ktl::delegate<Key>::signal;
	using OnFileDrop = ktl::delegate<std::span<str_t const>>::signal;

	static std::unique_ptr<Jukebox> make(GlfwInstance& instance, ktl::not_null<GLFWwindow*> window);

	Jukebox(Jukebox&&) = delete;
	Jukebox& operator=(Jukebox&&) = delete;

	Status tick(Time dt);

  private:
	struct Config {
		Props props;
		std::string path;

		Config() = default;
		Config(Config&&) = default;
		Config& operator=(Config&&) = default;
		~Config();
	};

	Jukebox(GlfwInstance& instance, ktl::not_null<GLFWwindow*> window);

	void mainControls();
	void seekBar();
	void trackControls();
	void tracklist();

	void playPause();
	void next();
	void prev();
	void seek(Time stamp);
	void muteUnmute();

	void loadConfig();
	void updateConfig();

	capo::Instance m_capo;
	char m_savePath[256] = "jukebox_playlist.txt";
	ktl::fixed_vector<Key, 16> m_keys;
	Player m_player;
	Controller m_controller;
	Config m_config;
	OnKey m_onKey;
	OnFileDrop m_onFileDrop;
	FileBrowser m_browser;
	LazySliderFloat m_seek;
	ktl::not_null<GLFWwindow*> m_window;
	bool m_saveFailure{};
	bool m_showImguiDemo{};
};
} // namespace jk
