#include <capo/capo.hpp>
#include <gui/imgui_instance.hpp>
#include <ktl/fixed_vector.hpp>
#include <misc/delta_time.hpp>
#include <misc/log.hpp>
#include <vk/boot.hpp>
#include <vk/renderer.hpp>
#include <win/glfw_instance.hpp>
#include <win/glfw_surface.hpp>
#include <imgui.h>

namespace jk {
struct Input {
	ktl::fixed_vector<Key, 16> keys;
};

void tick(GLFWwindow* window, Input const& input, [[maybe_unused]] jk::Time dt) {
	ImGui::ShowDemoWindow();
	for (Key const& key : input.keys) {
		if (key.press() && key.is(GLFW_KEY_SPACE) && key.any(GLFW_MOD_SHIFT | GLFW_MOD_CONTROL)) { Log::debug("space pressed"); }
	}
	if (jk_debug && Key::chord(input.keys, GLFW_KEY_W, GLFW_MOD_CONTROL)) { glfwSetWindowShouldClose(window, GLFW_TRUE); }
}
} // namespace jk

int main() {
	auto file = jk::Log::toFile("jukebox_log.txt");
	capo::Instance capoInst;
	if (!capoInst.valid()) { return 10; }
	auto glfwInst = jk::GlfwInstance::make();
	auto window = jk::WindowBuilder().size(600, 200).title("Jukebox").centre().show(false).make();
	auto boot = jk::VkBoot::make(jk::GlfwSurfaceMaker{window});
	if (!window || !boot) { return 10; }
	auto onKeySignal = glfwInst->onKey(window);
	jk::Input input;
	onKeySignal += [&input](jk::Key key) {
		if (input.keys.has_space()) { input.keys.push_back(key); }
	};
	auto getExtent = [window]() { return jk::framebufferSize(window); };
	jk::Renderer renderer(boot->gfx(), std::move(getExtent), glfwInst->onIconify(window));
	auto imguiInst = jk::ImGuiInstance::make(boot->gfx(), window, renderer.renderPass(), renderer.imageCount());
	if (!glfwInst || !imguiInst) { return 10; }
	ImGui::GetIO().FontGlobalScale = 1.33f;
	glfwShowWindow(window);
	jk::DeltaTime dt;
	while (!glfwWindowShouldClose(window)) {
		input = {};
		glfwInst->poll();
		imguiInst->beginFrame();
		jk::tick(window, input, ++dt);
		if (auto frame = renderer.nextFrame({})) {
			imguiInst->render(frame->cmd);
			renderer.present(*frame);
		}
	}
}
