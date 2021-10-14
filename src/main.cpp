#include <capo/capo.hpp>
#include <glfw_instance.hpp>
#include <glfw_surface.hpp>
#include <vk_boot.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

int main() {
	auto glfwInst = jk::GlfwInstance::make();
	auto window = jk::WindowBuilder().size(600, 200).title("Jukebox").centre().show(false).make();
	auto vkBoot = jk::VkBoot::make(jk::GlfwSurfaceMaker{window});
	if (!glfwInst || !window || !vkBoot) { return 10; }
	glfwShowWindow(window);
	while (!glfwWindowShouldClose(window)) { glfwPollEvents(); }
}
