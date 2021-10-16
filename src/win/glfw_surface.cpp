#include <vulkan/vulkan.hpp>

#include <misc/log.hpp>
#include <win/glfw_instance.hpp>
#include <win/glfw_surface.hpp>

namespace jk {
vk::SurfaceKHR GlfwSurfaceMaker::operator()(vk::Instance instance) const {
	VkSurfaceKHR vk_surface{};
	if (glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &vk_surface) != VK_SUCCESS) {
		Log::error("Failed to create Vulkan Surface");
	}
	return vk::SurfaceKHR(vk_surface);
}
} // namespace jk
