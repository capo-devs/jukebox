#include <capo/capo.hpp>
#include <glfw_instance.hpp>
#include <glfw_surface.hpp>
#include <imgui_instance.hpp>
#include <vk_boot.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

int main() {
	auto glfwInst = jk::GlfwInstance::make();
	auto window = jk::WindowBuilder().size(600, 200).title("Jukebox").centre().show(false).make();
	auto vkBoot = jk::VkBoot::make(jk::GlfwSurfaceMaker{window});
	auto imguiInst = jk::ImGuiInstance::make(vkBoot->gfx(), window);
	if (!glfwInst || !window || !vkBoot || !imguiInst) { return 10; }
	ImGui::GetIO().FontGlobalScale = 2.0f;
	glfwShowWindow(window);
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		if (auto f = imguiInst->frame()) {
			// do stuff
			ImGui::ShowDemoWindow();
		}
	}
}
