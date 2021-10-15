#include <glfw_instance.hpp>
#include <iostream>

namespace jk {
namespace {
void onGlfwError(int, char const* msg) { std::cerr << "GLFW Error: " << msg << "\n"; }
} // namespace

std::optional<GlfwInstance> GlfwInstance::make() noexcept {
	glfwSetErrorCallback(&onGlfwError);
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
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, m_flags.test(Flag::eFixedSize) ? GLFW_FALSE : GLFW_TRUE);
	auto ret = glfwCreateWindow(m_width, m_height, m_title.data(), nullptr, nullptr);
	if (ret) {
		if (m_flags.test(Flag::ePosition)) {
			glfwSetWindowPos(ret, m_x, m_y);
		} else if (m_flags.test(Flag::eCentre)) {
			glfwSetWindowPos(ret, (m_modeX - m_width) / 2, (m_modeY - m_height) / 2);
		}
		if (m_flags.test(Flag::eShow)) { glfwShowWindow(ret); }
	} else {
		std::cerr << "Failed to create window\n";
	}
	return ret;
}
} // namespace jk
