#include <glfw_instance.hpp>
#include <iostream>

namespace jk {
std::optional<GlfwInstance> GlfwInstance::make() noexcept {
	GlfwInstance ret;
	ret.m_init = glfwInit();
	if (!ret.m_init) {
		std::cerr << "Failed to init GLFW\n";
		return std::nullopt;
	}
	if (!glfwVulkanSupported()) {
		std::cerr << "Vulkan not supported\n";
		return std::nullopt;
	}
	return ret;
}

GlfwInstance::~GlfwInstance() noexcept {
	if (m_init) { glfwTerminate(); }
}

GLFWwindow* WindowBuilder::make() noexcept {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto ret = glfwCreateWindow(m_width, m_height, m_title.data(), nullptr, nullptr);
	if (ret) {
		if (m_position) {
			glfwSetWindowPos(ret, m_x, m_y);
		} else if (m_centre) {
			glfwSetWindowPos(ret, (m_modeX - m_width) / 2, (m_modeY - m_height) / 2);
		}
	} else {
		std::cerr << "Failed to create window\n";
	}
	return ret;
}
} // namespace jk
