#include <app/jukebox.hpp>
#include <capo/utils/format_unit.hpp>
#include <misc/buf_string.hpp>
#include <misc/log.hpp>
#include <win/glfw_instance.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace jk {
namespace {
template <std::size_t N = 256>
constexpr BufString<N> filename(std::string_view path) noexcept {
	if (path.empty()) { return "--"; }
	auto it = path.find_last_of('/');
	if (it == std::string_view::npos) { it = path.find_last_of('\\'); }
	if (it != std::string_view::npos) { path = path.substr(it + 1); }
	return path.size() < N ? path : path.substr(path.size() - N);
}

BufString<16> length(capo::utils::Length const& len) noexcept {
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
	return BufString<16>(fmt.data(), len.hours.count(), len.minutes.count(), len.seconds.count());
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

std::optional<float> SliderFloat::operator()(char const* name, float value, float min, float max, char const* label) {
	if (ImGui::SliderFloat(name, &value, min, max, label, ImGuiSliderFlags_NoInput)) {
		select = value;
		return std::nullopt;
	}
	return std::exchange(select, std::nullopt);
}

std::unique_ptr<Jukebox> Jukebox::make(GlfwInstance& instance, ktl::not_null<GLFWwindow*> window) {
	auto ret = std::unique_ptr<Jukebox>(new Jukebox(instance, window));
	if (!ret->m_capo.valid()) { return {}; }
	return ret;
}

Jukebox::Jukebox(GlfwInstance& instance, ktl::not_null<GLFWwindow*> window) : m_player(&m_capo), m_window(window) {
	m_onKey = instance.onKey(window);
	m_onFileDrop = instance.onFileDrop(window);
	m_onKey += [this](Key const& key) {
		if (m_keys.has_space()) { m_keys.push_back(key); }
	};
	m_onFileDrop += [this](std::span<str_t const> paths) {
		bool const empty = m_player.empty();
		if (m_player.add(paths) && empty) { m_player.play(); }
	};
	ImGui::GetStyle().ScaleAllSizes(1.33f);
	ImGui::GetIO().FontGlobalScale = 1.33f;
	ImGui::GetIO().IniFilename = {};
}

Jukebox::Status Jukebox::tick(Time) {
	m_player.update();
	auto keys = std::exchange(m_keys, {});
	for (Key const& key : keys) {
		if (key.press() && key.is(GLFW_KEY_SPACE) && key.any(GLFW_MOD_SHIFT | GLFW_MOD_CONTROL)) { Log::debug("space pressed"); }
		if (jk_debug && key.press() && key.is(GLFW_KEY_I) && key.all(GLFW_MOD_CONTROL)) { m_showImguiDemo = !m_showImguiDemo; }
	}
	if (jk_debug && Key::chord(keys, GLFW_KEY_W, GLFW_MOD_CONTROL)) { return Status::eQuit; }
	static constexpr auto flags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({0.0, 0.0f});
	auto const fb = framebufferSize(m_window);
	ImVec2 const size = {float(fb.x), float(fb.y)};
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("Jukebox", nullptr, flags)) {
		// main window
		ImGui::Text("%s", filename(m_player.path()).data());
		ImGui::Separator();

		bool const playing = m_player.playing();
		static constexpr float playWidth = 40.0f;
		ImVec2 const playSize = {playWidth, playWidth};
		ImVec2 const btnSize = {playWidth, playWidth * 0.75f};
		auto const playPauseBtn = [&]() { return playing ? ImGui::Button("||", playSize) : ImGui::ArrowButtonEx("play", ImGuiDir_Right, playSize); };
		if (playPauseBtn()) {
			if (playing) {
				m_player.pause();
			} else {
				m_player.play();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("_", btnSize)) { m_player.stop(); }
		ImGui::SameLine();
		if (ImGui::Button("<<", btnSize)) {
			if (m_player.isFirstTrack() || m_player.music().position() > Time(2s)) {
				m_player.seek({});
			} else {
				m_player.navPrev();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(">>", btnSize)) {
			if (m_player.isLastTrack()) {
				m_player.navFirst();
			} else {
				m_player.navNext();
			}
		}
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 240.0f - 20.0f);
		auto const volumeStr = m_player.muted() ? "<x##mute" : "<))##mute";
		if (ImGui::Button(volumeStr, {40.0f, 23.0f})) {
			if (m_player.muted()) {
				m_player.unmute();
			} else {
				m_player.mute();
			}
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200.0f);
		int gain = int(m_player.gain() * 100.0f);
		if (ImGui::SliderInt("##volume", &gain, 0, 100, "%1.2f")) { m_player.gain(float(gain) / 100.0f); }
		if (m_player.muted() && ImGui::IsItemClicked()) { m_player.unmute(); }

		auto const position = m_player.music().position();
		auto const total = m_player.music().meta().length();
		float pos = position.count();
		ImGui::Text("%s", length(capo::utils::Length(position)).data());
		ImGui::SameLine();
		auto totalLength = length(capo::utils::Length(total));
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(totalLength.data()).x - 10.0f);
		ImGui::Text("%s", totalLength.data());

		ImGui::SetNextItemWidth(-1.0f);
		if (auto seek = m_seek("##seek", pos, 0.0f, total.count())) { m_player.seek(capo::Time(*seek)); }

		ImGui::Separator();
		static ImVec2 const upDnSize = {25.0f, 25.0f};
		if (ImGui::ArrowButtonEx("move_down", ImGuiDir_Down, upDnSize)) { m_player.swapAhead(); }
		ImGui::SameLine();
		if (ImGui::ArrowButtonEx("move_up", ImGuiDir_Up, upDnSize)) { m_player.swapBehind(); }
		ImGui::SameLine();
		if (ImGui::Button("Clear", {0.0f, upDnSize.y})) { m_player.clear(); }
		if (ImGui::BeginChild("Playlist", {size.x - 20.0f, 0.0f}, true, ImGuiWindowFlags_HorizontalScrollbar)) {
			auto const& paths = m_player.paths();
			std::size_t idx{};
			std::optional<std::size_t> select;
			std::optional<std::string_view> pop;
			for (auto const& path : paths) {
				auto const file = filename(path);
				bool const selected = idx == m_player.head();
				if (ImGui::Selectable(file.data(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
					if (ImGui::GetIO().MouseDoubleClicked[0]) { select = idx; }
				}
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) { pop = path; }
				++idx;
			}
			ImGui::EndChild();
			if (select) {
				m_player.navIndex(*select);
			} else if (pop) {
				m_player.pop(*pop);
			}
		}
	}
	ImGui::End();
	if (m_showImguiDemo) { ImGui::ShowDemoWindow(&m_showImguiDemo); }
	return Status::eRun;
}
} // namespace jk
