#pragma once
#include <vk/types.hpp>
#include <optional>

struct GLFWwindow;

namespace jk {
class ImGuiInstance {
  public:
	static std::optional<ImGuiInstance> make(GFX const& gfx, GLFWwindow* window, vk::RenderPass pass, std::uint32_t imageCount);

	ImGuiInstance() = default;
	ImGuiInstance(ImGuiInstance&& rhs) noexcept : ImGuiInstance() { exchg(*this, rhs); }
	ImGuiInstance& operator=(ImGuiInstance rhs) noexcept { return (exchg(*this, rhs), *this); }
	~ImGuiInstance() noexcept;

	void beginFrame();
	void render(vk::CommandBuffer cb);

  private:
	static void exchg(ImGuiInstance& lhs, ImGuiInstance& rhs) noexcept;

	vk::Device m_device;
	Box<vk::DescriptorPool> m_pool;
	bool m_init{};
};
} // namespace jk
