#pragma once
#include <app/controller.hpp>
#include <app/player.hpp>
#include <app/props.hpp>
#include <ktl/delegate.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
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

	static std::optional<Jukebox> make(GlfwInstance& instance, ktl::not_null<GLFWwindow*> window);

	Jukebox(Jukebox&&) noexcept;
	Jukebox& operator=(Jukebox&&) noexcept;

	Status tick(Time dt);

  private:
	enum class Flag { eSaveFailure, eShowImGuiDemo };
	using Flags = ktl::enum_flags<Flag>;

	struct Config {
		Props props;
		std::string path;

		Config() = default;
		Config(Config&&) = default;
		Config& operator=(Config&&) = default;
		~Config();
	};

	Jukebox(GlfwInstance& glfw, ktl::not_null<GLFWwindow*> window, std::unique_ptr<capo::Instance>&& capo);

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

	void onKey(Key const& key);
	void onFileDrop(std::span<str_t const> paths);
	void replaceBindings() noexcept;

	// Ordered members
	std::unique_ptr<capo::Instance> m_capo;
	ktl::not_null<GLFWwindow*> m_window;
	Player m_player;
	Controller m_controller;

	struct {
		ktl::stack_string<256> savePath = "jukebox_playlist.txt";
		ktl::fixed_vector<Key, 16> keys;
		Config config;
		OnKey onKey;
		OnFileDrop onFileDrop;
		FileBrowser browser;
		LazySliderFloat seek;
		Flags flags;
	} m_data;
};
} // namespace jk
