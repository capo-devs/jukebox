#include <capo/capo.hpp>
#include <gui/imgui_instance.hpp>
#include <misc/delta_time.hpp>
#include <misc/log.hpp>
#include <vk/boot.hpp>
#include <vk/renderer.hpp>
#include <win/glfw_instance.hpp>
#include <win/glfw_surface.hpp>
#include <imgui.h>

namespace {
void tick([[maybe_unused]] jk::Time dt) { ImGui::ShowDemoWindow(); }
} // namespace

int main() {
	jk::Log::File log("jukebox_log.txt");
	capo::Instance capoInst;
	if (!capoInst.valid()) { return 10; }
	auto glfwInst = jk::GlfwInstance::make();
	auto window = jk::WindowBuilder().size(600, 200).title("Jukebox").centre().show(false).make();
	auto boot = jk::VkBoot::make(jk::GlfwSurfaceMaker{window});
	if (!window || !boot) { return 10; }
	jk::Renderer renderer(boot->gfx(), window);
	auto imguiInst = jk::ImGuiInstance::make(boot->gfx(), window, renderer.renderPass(), renderer.imageCount());
	if (!glfwInst || !imguiInst) { return 10; }
	ImGui::GetIO().FontGlobalScale = 1.33f;
	glfwShowWindow(window);
	jk::DeltaTime dt;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		if (auto frame = renderer.nextFrame({})) {
			imguiInst->beginFrame();
			tick(++dt);
			imguiInst->render(frame->cmd);
			renderer.submit(*frame);
		}
	}
}
