#pragma once
#include <optional>
#include <string_view>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace jk {
class GlfwInstance {
  public:
	static std::optional<GlfwInstance> make() noexcept;

	GlfwInstance() = default;
	GlfwInstance(GlfwInstance&& rhs) : GlfwInstance() { std::swap(m_init, rhs.m_init); }
	GlfwInstance& operator=(GlfwInstance rhs) { return (std::swap(m_init, rhs.m_init), *this); }
	~GlfwInstance() noexcept;

  private:
	bool m_init{};
};

class WindowBuilder {
  public:
	WindowBuilder& size(int width, int height) noexcept { return (m_width = width, m_height = height, *this); }
	WindowBuilder& title(std::string_view value) noexcept { return (m_title = value, *this); }
	WindowBuilder& show(bool value) noexcept { return (m_show = value, *this); }
	WindowBuilder& centre() noexcept {
		if (auto const mode = glfwGetVideoMode(glfwGetPrimaryMonitor())) {
			m_modeX = mode->width;
			m_modeY = mode->height;
			m_centre = true;
		}
		return *this;
	}
	WindowBuilder& position(int x, int y) noexcept {
		m_x = x;
		m_y = y;
		m_position = true;
		return *this;
	}

	GLFWwindow* make() noexcept;

  private:
	std::string_view m_title = "Untitled";
	int m_width = 64;
	int m_height = 64;
	int m_modeX = 0;
	int m_modeY = 0;
	int m_x = 0;
	int m_y = 0;
	bool m_centre{};
	bool m_position{};
	bool m_show = true;
};
} // namespace jk
