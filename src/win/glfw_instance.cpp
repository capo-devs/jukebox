#include <misc/log.hpp>
#include <win/glfw_instance.hpp>

namespace jk {
namespace {
void onGlfwError(int, char const* msg) { Log::error("GLFW Error: {}", msg); }
} // namespace

UVec2 framebufferSize(GLFWwindow* window) noexcept {
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	return {std::uint32_t(w), std::uint32_t(h)};
}

UVec2 windowSize(GLFWwindow* window) noexcept {
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	return {std::uint32_t(w), std::uint32_t(h)};
}

std::optional<GlfwInstance> GlfwInstance::make() noexcept {
	glfwSetErrorCallback(&onGlfwError);
	GlfwInstance ret;
	ret.m_init = glfwInit();
	if (!ret.m_init) {
		Log::error("Failed to init GLFW");
		return std::nullopt;
	}
	if (!glfwVulkanSupported()) {
		Log::error("Vulkan not supported");
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
		Log::error("Failed to create window");
	}
	return ret;
}
} // namespace jk
