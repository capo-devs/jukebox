#pragma once
#include <vk_boot.hpp>
#include <optional>

struct GLFWwindow;

namespace jk {
class ImGuiFrame {
  public:
	~ImGuiFrame();

	explicit operator bool() const noexcept { return true; }

  private:
	ImGuiFrame();

	friend class ImGuiInstance;
};

class ImGuiInstance {
  public:
	static std::optional<ImGuiInstance> make(GFX const& gfx, GLFWwindow* window);

	ImGuiInstance() = default;
	ImGuiInstance(ImGuiInstance&& rhs) noexcept : ImGuiInstance() { exchg(*this, rhs); }
	ImGuiInstance& operator=(ImGuiInstance rhs) noexcept { return (exchg(*this, rhs), *this); }
	~ImGuiInstance() noexcept;

	ImGuiFrame frame();

	static vk::Extent2D framebufferSize(GLFWwindow* window);
	static vk::Extent2D windowSize(GLFWwindow* window);

  private:
	static void exchg(ImGuiInstance& lhs, ImGuiInstance& rhs) noexcept;

	GFX m_gfx;
	vk::DescriptorPool m_pool;
	bool m_init{};
};
} // namespace jk
