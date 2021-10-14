#pragma once
#include <glfw_instance.hpp>
#include <vk_surface_maker.hpp>

namespace jk {
struct GlfwSurfaceMaker : SurfaceMaker {
	GLFWwindow* window;

	GlfwSurfaceMaker(GLFWwindow* window) noexcept : window(window) {}

	vk::SurfaceKHR operator()(vk::Instance instance) const override;
};
} // namespace jk
