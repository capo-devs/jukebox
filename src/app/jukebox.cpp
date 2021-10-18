#include <app/jukebox.hpp>
#include <capo/utils/format_unit.hpp>
#include <misc/buf_string.hpp>
#include <misc/log.hpp>
#include <win/glfw_instance.hpp>
#include <imgui.h>

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
} // namespace

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
	m_onFileDrop += [this](std::span<str_t const> paths) { m_player.add(paths, true, true); };
	ImGui::GetStyle().ScaleAllSizes(1.33f);
	ImGui::GetIO().FontGlobalScale = 1.33f;
}

Jukebox::Status Jukebox::tick(Time) {
	m_player.update();
	auto keys = std::exchange(m_keys, {});
	for (Key const& key : keys) {
		if (key.press() && key.is(GLFW_KEY_SPACE) && key.any(GLFW_MOD_SHIFT | GLFW_MOD_CONTROL)) { Log::debug("space pressed"); }
		if (jk_debug && key.press() && key.is(GLFW_KEY_I) && key.all(GLFW_MOD_CONTROL)) { m_showImguiDemo = !m_showImguiDemo; }
	}
	if (jk_debug && Key::chord(keys, GLFW_KEY_W, GLFW_MOD_CONTROL)) { return Status::eQuit; }
	static constexpr auto flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
								  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({0.0, 0.0f});
	auto const fb = framebufferSize(m_window);
	ImVec2 const size = {float(fb.x), float(fb.y)};
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("Jukebox", nullptr, flags)) {
		// main window
		ImGui::Text("%s", filename(m_player.path()).data());
		ImGui::Separator();

		auto const status = m_player.status();
		bool const playing = status == Player::Status::ePlaying;
		auto const button = playing ? "||" : "|>";
		if (ImGui::Button(button)) {
			if (playing) {
				m_player.pause();
			} else {
				m_player.play();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("[]")) { m_player.stop(); }
		ImGui::SameLine();
		if (ImGui::Button("<<")) {
			if (m_player.music().position() > Time(2s)) {
				m_player.seek({});
			} else {
				if (m_player.index() == 0) {
					m_player.navLast(playing);
				} else {
					m_player.navPrev(playing);
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(">>")) {
			if (m_player.isLastTrack()) {
				m_player.navFirst(playing);
			} else {
				m_player.navNext(playing);
			}
		}

		auto const position = m_player.music().position();
		auto const total = m_player.music().meta().length();
		float pos = position.count();
		auto bar = m_seek ? *m_seek : position;
		ImGui::Text("%s", length(capo::utils::Length(bar)).data());
		ImGui::SameLine();
		if (ImGui::SliderFloat(length(total).data(), &pos, 0.0f, total.count(), "", ImGuiSliderFlags_NoInput)) {
			m_seek = capo::Time(pos);
		} else if (m_seek) {
			m_player.seek(*m_seek);
			m_seek.reset();
		}

		ImGui::Separator();
		float gain = m_player.music().gain();
		if (ImGui::SliderFloat("<))", &gain, 0.0f, 1.0f, "%1.2f")) { m_player.gain(gain); }

		ImGui::Separator();
		if (ImGui::BeginChild("Playlist", {size.x - 20.0f, 0.0f}, true, ImGuiWindowFlags_HorizontalScrollbar)) {
			auto const& paths = m_player.paths();
			std::size_t idx{};
			std::optional<std::size_t> select;
			std::optional<std::string_view> pop;
			for (auto const& path : paths) {
				auto const file = filename(path);
				bool const selected = idx == m_player.index();
				if (ImGui::Selectable(file.data(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
					if (ImGui::GetIO().MouseDoubleClicked[0]) { select = idx; }
				}
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) { pop = path; }
				++idx;
			}
			ImGui::EndChild();
			if (select) {
				m_player.navIndex(*select, true);
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
