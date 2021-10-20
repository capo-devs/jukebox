#include <app/jukebox.hpp>
#include <gui/imgui_instance.hpp>
#include <ktl/fixed_vector.hpp>
#include <misc/delta_time.hpp>
#include <misc/log.hpp>
#include <version.hpp>
#include <vk/boot.hpp>
#include <vk/renderer.hpp>
#include <win/glfw_instance.hpp>
#include <win/glfw_surface.hpp>
#include <imgui.h>

namespace {
[[maybe_unused]] constexpr std::string_view shortVersion() noexcept {
	auto ret = jukebox_version;
	auto it = ret.find('.');
	it = ret.find('.', it + 1);
	return ret.substr(0, it);
}

std::string windowTitle(std::string_view appName) {
	if constexpr (jk_debug) {
		return ktl::format("{} v{} | Debug", appName, jukebox_version);
	} else {
		return ktl::format("{} v{}", appName, shortVersion());
	}
}
} // namespace

int main() {
	auto file = jk::Log::toFile("jukebox_log.txt");
	auto glfwInst = jk::GlfwInstance::make();
	auto window = jk::WindowBuilder().size(650, 300).title(windowTitle("Jukebox")).centre().show(false).make();
	auto boot = jk::VkBoot::make(jk::GlfwSurfaceMaker{window});
	if (!window || !boot) { return 10; }
	auto onKeySignal = glfwInst->onKey(window);
	auto getExtent = [window]() { return jk::framebufferSize(window); };
	jk::Renderer renderer(boot->gfx(), std::move(getExtent), glfwInst->onIconify(window));
	auto imguiInst = jk::ImGuiInstance::make(boot->gfx(), window, renderer.renderPass(), renderer.imageCount());
	auto jukebox = jk::Jukebox::make(*glfwInst, window);
	if (!glfwInst || !imguiInst || !jukebox) { return 10; }
	glfwShowWindow(window);
	jk::DeltaTime dt;
	bool quit = false;
	while (!quit && !glfwWindowShouldClose(window)) {
		glfwInst->poll();
		imguiInst->beginFrame();
		// jk::tick(window, input, ++dt);
		quit = jukebox->tick(dt) == jk::Jukebox::Status::eQuit;
		if (auto frame = renderer.nextFrame({})) {
			imguiInst->render(frame->cmd);
			renderer.present(*frame);
		}
	}
}
