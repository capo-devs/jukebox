#include <app/jukebox.hpp>
#include <app/playlist.hpp>
#include <capo/utils/format_unit.hpp>
#include <ktl/stack_string.hpp>
#include <misc/log.hpp>
#include <misc/vec.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>
#include <map>
#include <set>

namespace jk {
namespace stdfs = std::filesystem;

template <typename T>
std::ostream& operator<<(std::ostream& out, TVec2<T> vec) {
	return out << vec.x << 'x' << vec.y;
}

template <typename T>
std::istream& operator>>(std::istream& in, TVec2<T>& out) {
	char discard;
	in >> out.x >> discard >> out.y;
	return in;
}

namespace {
template <std::size_t N = 256>
constexpr ktl::stack_string<N> filename(std::string_view path, bool ext) noexcept {
	if (path.empty()) { return "--"; }
	auto it = path.find_last_of('/');
	if (it == std::string_view::npos) { it = path.find_last_of('\\'); }
	if (it != std::string_view::npos) { path = path.substr(it + 1); }
	if (it = path.find_last_of('.'); !ext && it != std::string_view::npos) { path = path.substr(0, it); }
	return path.size() < N ? path : path.substr(path.size() - N);
}

ktl::stack_string<16> length(capo::utils::Length const& len) noexcept {
	static constexpr std::string_view options[] = {"%ld:0%ld:0%ld", "%ld:0%ld:%ld", "%ld:%ld:0%ld"};
	std::string_view fmt = "%ld:%ld:%ld";
	if (len.minutes < stdch::minutes(10)) {
		if (len.seconds < 10s) {
			fmt = options[0];
		} else {
			fmt = options[1];
		}
	} else {
		if (len.seconds < 10s) { fmt = options[2]; }
	}
	return ktl::stack_string<16>(fmt.data(), len.hours.count(), len.minutes.count(), len.seconds.count());
}

UVec2 framebufferSize(GLFWwindow* window) noexcept {
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	return {std::uint32_t(w), std::uint32_t(h)};
}

UVec2 windowSize(GLFWwindow* window) noexcept {
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	return {std::uint32_t(w), std::uint32_t(h)};
}

IVec2 windowPos(GLFWwindow* window) noexcept {
	int w, h;
	glfwGetWindowPos(window, &w, &h);
	return {std::int32_t(w), std::int32_t(h)};
}

[[maybe_unused]] void tooltipMarker(char const* desc, char const* marker = "(?)") {
	ImGui::SameLine();
	ImGui::TextDisabled("%s", marker);
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}
} // namespace

std::optional<float> LazySliderFloat::operator()(char const* name, float value, float min, float max, char const* label) {
	if (ImGui::SliderFloat(name, &value, min, max, label, ImGuiSliderFlags_NoInput)) {
		prev = value;
		return std::nullopt;
	}
	return std::exchange(prev, std::nullopt);
}

struct FileBrowser::Impl {
	enum Ext { eFlac, eMp3, eWav, eTxt };
	stdfs::path m_pwd;
	bool exts[4] = {true, true, true, false};

	stdfs::path operator()(bool& out_show) {
		stdfs::path ret;
		if (out_show) {
			ImGui::SetNextWindowSize({450.0f, 200.0f}, ImGuiCond_Once);
			if (ImGui::Begin(jk::FileBrowser::title_v.data(), &out_show)) {
				ImGui::Text("%s", m_pwd.generic_string().data());
				ImGui::Checkbox("FLAC", &exts[eFlac]);
				ImGui::SameLine();
				ImGui::Checkbox("MP3", &exts[eMp3]);
				ImGui::SameLine();
				ImGui::Checkbox("WAV", &exts[eWav]);
				ImGui::SameLine();
				ImGui::Checkbox("Playlist", &exts[eTxt]);
				ktl::fixed_vector<std::string_view, 4> extNames;
				if (exts[eFlac]) { extNames.push_back(".flac"); }
				if (exts[eMp3]) { extNames.push_back(".mp3"); }
				if (exts[eWav]) { extNames.push_back(".wav"); }
				if (exts[eTxt]) { extNames.push_back(".txt"); }
				std::set<stdfs::path> dirs;
				std::map<std::string, std::set<stdfs::path>> files;
				for (auto const& path : stdfs::directory_iterator(m_pwd)) {
					if (path.is_directory()) {
						auto p = path.path();
						if (!p.filename().generic_string().starts_with('.')) { dirs.insert(std::move(p)); }
					} else {
						auto p = path.path();
						auto const ext = p.extension().string();
						if (std::find(extNames.begin(), extNames.end(), ext) != extNames.end()) {
							if (ext != ".txt" || Playlist::valid(p.string().data(), true)) { files[ext].insert(std::move(p)); }
						}
					}
				}
				if (ImGui::BeginChild("Playlist", {ImGui::GetWindowSize().x - 20.0f, 0.0f}, true, ImGuiWindowFlags_HorizontalScrollbar)) {
					if (m_pwd.has_parent_path() && ImGui::Selectable("..##go_up", false)) {
						pwd(m_pwd.parent_path());
					} else {
						for (auto& path : dirs) {
							std::string const name = path.filename().generic_string() + '/';
							if (ImGui::Selectable(name.data(), false)) { pwd(std::move(path)); }
						}
						for (auto& [_, set] : files) {
							for (auto const& path : set) {
								if (ImGui::Selectable(path.filename().generic_string().data(), false)) { ret = std::move(path); }
							}
						}
					}
				}
				ImGui::EndChild();
			}
			ImGui::End();
		}
		return ret;
	}

	void pwd(stdfs::path path) { m_pwd = std::move(path); }
};

FileBrowser::FileBrowser() : m_impl(std::make_unique<Impl>()) { m_impl->m_pwd = stdfs::current_path(); }
FileBrowser::FileBrowser(FileBrowser&&) noexcept = default;
FileBrowser& FileBrowser::operator=(FileBrowser&&) noexcept = default;
FileBrowser::~FileBrowser() noexcept = default;

std::string FileBrowser::operator()() { return (*m_impl)(m_show).generic_string(); }

std::optional<Jukebox> Jukebox::make(ktl::not_null<GLFWwindow*> window) {
	auto capo = std::make_unique<capo::Instance>();
	if (!capo->valid()) {
		Log::error("[Jukebox] Failed to initialize capo instance!");
		return {};
	}
	return Jukebox(window, std::move(capo));
}

Jukebox::Jukebox(ktl::not_null<GLFWwindow*> window, std::unique_ptr<capo::Instance>&& capo)
	: m_capo(std::move(capo)), m_window(window), m_player(m_capo.get()) {
	ImGui::GetStyle().ScaleAllSizes(1.33f);
	ImGui::GetIO().FontGlobalScale = 1.33f;
	ImGui::GetIO().IniFilename = {};
	loadConfig();
}

void Jukebox::onKey(Key const& key) {
	if (m_data.keys.has_space()) { m_data.keys.push_back(key); }
	m_controller.onKey(key);
}

void Jukebox::onFileDrop(std::span<str_t const> paths) {
	bool const empty = m_player.empty();
	if (m_player.add(paths) && empty) { m_player.play(); }
}

Jukebox::Status Jukebox::tick(Time) {
	m_player.update();
	using Action = Controller::Action;
	for (auto const& response : m_controller.responses()) {
		switch (response.action) {
		case Action::ePlayPause: playPause(); break;
		case Action::eStop: m_player.stop(); break;
		case Action::eMute: muteUnmute(); break;
		case Action::eNext: next(); break;
		case Action::ePrev: prev(); break;
		case Action::eSeek: seek(capo::Time(response.value)); break;
		case Action::eVolume: m_player.gain(std::clamp(m_player.gain() + response.value, 0.0f, 1.0f)); break;
		case Action::eQuit: return Status::eQuit;
		case Action::eNone: break;
		}
	}
	auto keys = std::exchange(m_data.keys, {});
	if constexpr (jk_debug) {
		for (Key const& key : keys) {
			if (key.press() && key.is(GLFW_KEY_I) && key.all(GLFW_MOD_CONTROL)) { m_data.flags.flip(Flag::eShowImGuiDemo); }
		}
	}
	static constexpr auto flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({0.0, 0.0f});
	auto const fb = framebufferSize(m_window);
	ImGui::SetNextWindowSize({float(fb.x), float(fb.y)});
	if (ImGui::Begin("Jukebox", nullptr, flags)) {
		// main window
		ImGui::Text("%s", filename(m_player.path(), false).data());
		ImGui::Separator();
		mainControls();
		seekBar();
		ImGui::Separator();
		trackControls();
		ImGui::Separator();
		tracklist();
	}
	ImGui::End();
	if constexpr (jk_debug) {
		if (m_data.flags[Flag::eShowImGuiDemo]) {
			bool b = true;
			ImGui::ShowDemoWindow(&b);
			m_data.flags.assign(Flag::eShowImGuiDemo, b);
		}
	}
	updateConfig();
	return Status::eRun;
}

void Jukebox::mainControls() {
	static constexpr float playWidth = 40.0f;
	ImVec2 const playBtnSize = {playWidth, playWidth};
	ImVec2 const btnSize = {playWidth, playWidth * 0.75f};
	float stopOffsetX{};
	auto const playPauseBtn = [&]() {
		return m_player.playing() ? ImGui::Button("||##pause", playBtnSize) : ImGui::ArrowButtonEx("play", ImGuiDir_Right, playBtnSize);
	};
	if (playPauseBtn()) { playPause(); }
	stopOffsetX += playBtnSize.x + 10.0f;
	ImGui::SameLine();
	if (ImGui::Button("##stop", btnSize)) { m_player.stop(); }
	{
		float const rect = 10.0f;
		ImVec2 const c = ImGui::GetCursorPos();
		ImVec2 const p_min = {c.x + stopOffsetX + (playBtnSize.x - rect) * 0.5f, c.y - (playBtnSize.x - rect) - 5.0f};
		ImVec2 const p_max = {p_min.x + rect, p_min.y + rect};
		ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, 0xffffffff);
	}
	ImGui::SameLine();
	if (ImGui::Button("<<##previous", btnSize)) { prev(); }
	ImGui::SameLine();
	if (ImGui::Button(">>##next", btnSize)) { next(); }
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 240.0f - 20.0f);
	auto const volumeStr = m_player.muted() ? "<x##mute" : "<))##mute";
	if (ImGui::Button(volumeStr, {40.0f, 23.0f})) { muteUnmute(); }
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	int gain = int(m_player.gain() * 100.0f);
	if (ImGui::SliderInt("##volume", &gain, 0, 100, "%1.2f")) { m_player.gain(float(gain) / 100.0f); }
	if (m_player.muted() && ImGui::IsItemClicked()) { m_player.unmute(); }
	if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
		m_player.gain(std::clamp(m_player.gain() + ImGui::GetIO().MouseWheel * 0.1f, 0.0f, 1.0f));
	}
}

void Jukebox::seekBar() {
	auto const position = m_player.music().position();
	auto const total = m_player.music().meta().length();
	float pos = position.count();
	ImGui::Text("%s", length(capo::utils::Length(position)).data());
	ImGui::SameLine();
	auto totalLength = length(capo::utils::Length(total));
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(totalLength.data()).x - 10.0f);
	ImGui::Text("%s", totalLength.data());
	ImGui::SetNextItemWidth(-1.0f);
	if (auto seek = m_data.seek("##seek", pos, 0.0f, total.count())) { m_player.seek(capo::Time(*seek)); }
}

void Jukebox::trackControls() {
	static ImVec2 const upDnSize = {25.0f, 25.0f};
	if (ImGui::ArrowButtonEx("move_down", ImGuiDir_Down, upDnSize)) { m_player.swapAhead(); }
	ImGui::SameLine();
	if (ImGui::ArrowButtonEx("move_up", ImGuiDir_Up, upDnSize)) { m_player.swapBehind(); }
	ImGui::SameLine();
	if (ImGui::Button("+##push", upDnSize)) { m_data.browser.m_show = !m_data.browser.m_show; }
	if (!m_player.empty()) {
		ImGui::SameLine();
		if (ImGui::Button("Clear", {0.0f, upDnSize.y})) { m_player.clear(); }
		ImGui::SameLine();
		if (ImGui::Button("Save", {0.0f, upDnSize.y})) {
			ImGui::OpenPopup("save_playlist");
			m_data.flags.reset(Flag::eSaveFailure);
		}
		if (ImGui::BeginPopup("save_playlist")) {
			if (m_data.flags[Flag::eSaveFailure]) {
				auto text = ktl::stack_string<512>("Save to %s failed!", m_data.savePath);
				ImGui::Text("%s", text.data());
			} else {
				ImGui::Text("Path:");
				ImGui::SameLine();
				ImGui::InputText("##save_path", m_data.savePath.c_str(), m_data.savePath.capacity());
			}
			if (ImGui::Button("OK")) {
				Playlist list{{m_player.paths().begin(), m_player.paths().end()}};
				if (!m_data.flags[Flag::eSaveFailure] && !list.save(m_data.savePath.data())) {
					m_data.flags.set(Flag::eSaveFailure);
				} else {
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}
	if (auto path = m_data.browser(); !path.empty()) { m_player.push(std::move(path), false); }
	ImGui::SameLine();
	bool preload = m_player.mode() == Player::Mode::ePreload;
	if (ImGui::Checkbox("Preload", &preload)) { m_player.mode(preload ? Player::Mode::ePreload : Player::Mode::eStream); }
	ImGui::SameLine();
	tooltipMarker("Faster to seek, slower to open");
}

void Jukebox::tracklist() {
	ImGui::Text("Playlist");
	tooltipMarker("Drag files / use + to add\nLeft click to play\nRight click to remove");
	if (ImGui::BeginChild("Playlist", {ImGui::GetWindowSize().x - 20.0f, 0.0f}, true, ImGuiWindowFlags_HorizontalScrollbar)) {
		auto const& paths = m_player.paths();
		std::size_t idx{};
		std::optional<std::size_t> select;
		std::optional<std::string_view> pop;
		for (auto const& path : paths) {
			auto const file = filename(path, true);
			bool const selected = idx == m_player.head();
			if (ImGui::Selectable(file.data(), selected)) { select = idx; }
			if (!select && ImGui::IsItemClicked(ImGuiMouseButton_Right)) { pop = path; }
			++idx;
		}
		if (select) {
			m_player.navIndex(*select);
			if (!m_player.playing()) { m_player.play(); }
		} else if (pop) {
			m_player.pop(*pop);
		}
	}
	ImGui::EndChild();
}

void Jukebox::playPause() {
	if (m_player.playing()) {
		m_player.pause();
	} else {
		m_player.play();
	}
}

void Jukebox::next() {
	if (m_player.isLastTrack()) {
		m_player.navFirst();
	} else {
		m_player.navNext();
	}
}

void Jukebox::prev() {
	if (m_player.isFirstTrack() || m_player.music().position() > Time(2s)) {
		m_player.seek({});
	} else {
		m_player.navPrev();
	}
}

void Jukebox::seek(Time delta) {
	auto const remain = m_player.music().meta().length() - m_player.music().position();
	if (delta >= remain) {
		if (m_player.isLastTrack()) {
			m_player.stop();
		} else {
			next();
		}
	}
	m_player.seek(m_player.music().position() + delta);
}

void Jukebox::muteUnmute() {
	if (m_player.muted()) {
		m_player.unmute();
	} else {
		m_player.mute();
	}
}

void Jukebox::loadConfig() {
	m_data.config.path = "jukebox_config.ini";
	if (m_data.config.props.load(m_data.config.path.data())) {
		m_player.gain(float(m_data.config.props.get<int>("volume", 100)) / 100.0f);
		if (m_data.config.props.contains("window_size")) {
			auto const size = m_data.config.props.get<UVec2>("window_size");
			glfwSetWindowSize(m_window, int(size.x), int(size.y));
		}
		if (m_data.config.props.contains("window_pos")) {
			auto const pos = m_data.config.props.get<UVec2>("window_pos");
			glfwSetWindowPos(m_window, int(pos.x), int(pos.y));
		}
		Log::info("[Jukebox] Loaded config from [{}]", m_data.config.path);
	}
}

void Jukebox::updateConfig() {
	m_data.config.props.add(true, "volume", int(m_player.gain() * 100.0f));
	m_data.config.props.add(true, "window_size", windowSize(m_window));
	m_data.config.props.add(true, "window_pos", windowPos(m_window));
}

Jukebox::Config::~Config() {
	if (!path.empty() && !props.empty()) {
		if (props.save(path.data())) {
			Log::info("[Jukebox] Config saved to [{}]", path);
		} else {
			Log::warn("[Jukebox] Failed to save config to [{}]", path);
		}
	}
}
} // namespace jk
