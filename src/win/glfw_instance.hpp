#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <misc/delegate.hpp>
#include <misc/vec.hpp>
#include <win/key.hpp>
#include <optional>
#include <string_view>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace jk {
UVec2 framebufferSize(GLFWwindow* window) noexcept;
UVec2 windowSize(GLFWwindow* window) noexcept;

using OnKey = Delegate<Key>;
using OnIconify = Delegate<bool>;

class GlfwInstance {
  public:
	static std::optional<GlfwInstance> make() noexcept;

	GlfwInstance() = default;
	GlfwInstance(GlfwInstance&& rhs) noexcept : GlfwInstance() { std::swap(m_init, rhs.m_init); }
	GlfwInstance& operator=(GlfwInstance rhs) noexcept { return (std::swap(m_init, rhs.m_init), *this); }
	~GlfwInstance() noexcept;

	OnKey::Signal onKey(GLFWwindow* window);
	OnIconify::Signal onIconify(GLFWwindow* window);
	void poll() noexcept;

  private:
	bool m_init{};
};

class WindowBuilder {
  public:
	WindowBuilder& size(int width, int height) noexcept { return (m_width = width, m_height = height, *this); }
	WindowBuilder& title(std::string_view value) noexcept { return (m_title = value, *this); }
	WindowBuilder& show(bool value = true) noexcept { return (m_flags.assign(Flag::eShow, value), *this); }
	WindowBuilder& fixedSize(bool value = true) noexcept { return (m_flags.assign(Flag::eFixedSize, value), *this); }
	WindowBuilder& centre() noexcept {
		if (auto const mode = glfwGetVideoMode(glfwGetPrimaryMonitor())) {
			m_modeX = mode->width;
			m_modeY = mode->height;
			m_flags.set(Flag::eCentre);
		}
		return *this;
	}
	WindowBuilder& position(int x, int y) noexcept {
		m_x = x;
		m_y = y;
		m_flags.set(Flag::ePosition);
		return *this;
	}

	GLFWwindow* make() noexcept;

  private:
	enum class Flag { eShow, eCentre, ePosition, eFixedSize };
	using Flags = ktl::enum_flags<Flag>;

	std::string_view m_title = "Untitled";
	int m_width = 64;
	int m_height = 64;
	int m_modeX = 0;
	int m_modeY = 0;
	int m_x = 0;
	int m_y = 0;
	Flags m_flags;
};
} // namespace jk
