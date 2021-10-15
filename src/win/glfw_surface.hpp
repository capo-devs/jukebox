#pragma once
#include <vk/types.hpp>

struct GLFWwindow;

namespace jk {
struct GlfwSurfaceMaker : SurfaceMaker {
	GLFWwindow* window;

	GlfwSurfaceMaker(GLFWwindow* window) noexcept : window(window) {}

	vk::SurfaceKHR operator()(vk::Instance instance) const override;
};
} // namespace jk
