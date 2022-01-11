#pragma once
#include <app/controller.hpp>
#include <app/player.hpp>
#include <app/props.hpp>
#include <dibs/event.hpp>
#include <ktl/delegate.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <ktl/fixed_vector.hpp>
#include <memory>
#include <optional>

struct GLFWwindow;

namespace jk {
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

	static std::optional<Jukebox> make(ktl::not_null<GLFWwindow*> window);

	void onKey(dibs::Event::Key const& key);
	void onFileDrop(std::span<std::string const> paths);

	void update();

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

	Jukebox(ktl::not_null<GLFWwindow*> window, std::unique_ptr<capo::Instance>&& capo);

	void mainControls();
	void seekBar();
	void trackControls();
	void tracklist();

	void playPause();
	void next();
	void prev();
	void seek(capo::Time stamp);
	void muteUnmute();

	void loadConfig();
	void updateConfig();

	// Ordered members
	std::unique_ptr<capo::Instance> m_capo;
	ktl::not_null<GLFWwindow*> m_window;
	Player m_player;
	Controller m_controller;

	struct {
		ktl::stack_string<256> savePath = "jukebox_playlist.txt";
		Config config;
		FileBrowser browser;
		LazySliderFloat seek;
		Flags flags;
	} m_data;
};
} // namespace jk
