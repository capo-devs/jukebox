#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <misc/delegate.hpp>
#include <vk/swapchain.hpp>
#include <optional>

namespace jk {
class Renderer {
  public:
	using Clear = std::array<float, 4>;
	using IconifySignal = Delegate<bool>::Signal;
	static constexpr std::size_t buffering_v = 2;

	Renderer(GFX const& gfx, Swapchain::GetExtent&& getExtent, IconifySignal&& onIconify);

	std::optional<RenderFrame> nextFrame(Clear const& clear);
	bool present(RenderFrame const& frame);

	vk::RenderPass renderPass() const noexcept { return m_pass; }
	std::uint32_t imageCount() const noexcept { return std::uint32_t(m_factory.swapchain().images.size()); }

  private:
	enum class Flag { ePaused, eRebuild };
	using Flags = ktl::enum_flags<Flag>;

	static Box<vk::RenderPass> makeRenderPass(vk::Device device, vk::Format colour);
	static void cycleFence(Box<vk::Fence>& out);
	void cycleFramebuffer(Box<vk::Framebuffer>& out, RenderImage const& image);
	void onIconify(bool iconified);

	bool swapchainCheck(vk::Result result);
	void resync();
	bool paused() const noexcept;

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
	IconifySignal m_onIconify;
	Flags m_flags;
};
} // namespace jk
