#pragma once
#include <vk/swapchain.hpp>
#include <optional>
#include <unordered_set>

namespace jk {
class Renderer {
  public:
	using Clear = std::array<float, 4>;
	static constexpr std::size_t buffering_v = 2;

	Renderer(GFX const& gfx, GLFWwindow* window);

	std::optional<RenderFrame> nextFrame(Clear const& clear);
	bool submit(RenderFrame const& frame);

	vk::RenderPass renderPass() const noexcept { return m_pass; }
	std::uint32_t imageCount() const noexcept { return std::uint32_t(m_factory.swapchain().images.size()); }

  private:
	static void onIconify(GLFWwindow* window, int iconified) noexcept;
	static Box<vk::RenderPass> makeRenderPass(vk::Device device, vk::Format colour);
	static void cycleFence(Box<vk::Fence>& out);
	void cycleFramebuffer(Box<vk::Framebuffer>& out, RenderImage const& image);

	bool swapchainCheck(vk::Result result);

	inline static std::unordered_set<GLFWwindow*> s_rebuild;

	struct Sync {
		RenderFrame frame;
		Box<vk::Framebuffer> framebuffer;
		Box<vk::CommandPool> pool;
		Box<vk::Semaphore> draw;
		Box<vk::Semaphore> present;
		Box<vk::Fence> drawn;
	};

	Swapchain::Factory m_factory;
	RingBuffer<Sync, buffering_v> m_sync;
	Box<vk::RenderPass> m_pass;
	std::optional<Swapchain::Acquire> m_acquired;
};
} // namespace jk
