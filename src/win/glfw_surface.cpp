#include <vulkan/vulkan.hpp>

#include <win/glfw_instance.hpp>
#include <win/glfw_surface.hpp>
#include <iostream>

namespace jk {
vk::SurfaceKHR GlfwSurfaceMaker::operator()(vk::Instance instance) const {
	VkSurfaceKHR vk_surface{};
	if (glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &vk_surface) != VK_SUCCESS) {
		std::cerr << "Failed to create Vulkan Surface\n";
	}
	return vk::SurfaceKHR(vk_surface);
}
} // namespace jk
