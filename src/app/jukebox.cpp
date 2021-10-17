#include <app/jukebox.hpp>
#include <capo/utils/format_unit.hpp>
#include <misc/log.hpp>
#include <win/glfw_instance.hpp>
#include <imgui.h>

namespace jk {
namespace {
std::string filename(std::string_view path) {
	if (path.empty()) { return "--"; }
	auto it = path.find_last_of('/');
	if (it == std::string_view::npos) { it = path.find_last_of('\\'); }
	if (it != std::string_view::npos) { return std::string(path.substr(it + 1)); }
	return std::string(path);
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
				m_player.navPrev(m_player.status() == Player::Status::ePlaying);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(">>")) { m_player.navNext(m_player.status() == Player::Status::ePlaying); }

		auto const position = m_player.music().position();
		auto const length = m_player.music().meta().length();
		auto const time = capo::utils::Length(position);
		float pos = position.count();
		ImGui::Text("%s", ktl::format("{}", capo::utils::Length(time)).data());
		ImGui::SameLine();
		if (ImGui::SliderFloat("##seek_bar", &pos, 0.0f, length.count(), "", ImGuiSliderFlags_NoInput)) { m_player.seek(Time(pos)); }
		ImGui::SameLine();
		ImGui::Text("%s", ktl::format("{}", capo::utils::Length(length)).data());

		ImGui::Separator();
		float gain = m_player.music().gain();
		if (ImGui::SliderFloat("<))", &gain, 0.0f, 1.0f, "%1.2f")) { m_player.gain(gain); }

		ImGui::Separator();
		if (ImGui::BeginChild("Playlist", {size.x - 20.0f, 0.0f}, true, ImGuiWindowFlags_HorizontalScrollbar)) {
			auto const& paths = m_player.paths();
			if (paths.empty()) {
				ImGui::SetNextItemWidth((float)fb.x);
				ImGui::Text("--");
			}
			std::size_t idx{};
			for (auto const& path : paths) {
				ImGui::SetNextItemWidth((float)fb.x);
				if (idx++ == m_player.index()) {
					ImGui::TextColored({0.7f, 0.5f, 1.0f, 1.0f}, "%s", path.data());
				} else {
					ImGui::Text("%s", path.data());
				}
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();
	if (m_showImguiDemo) { ImGui::ShowDemoWindow(&m_showImguiDemo); }
	return Status::eRun;
}
} // namespace jk
