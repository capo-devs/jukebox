#pragma once
#include <ktl/fixed_vector.hpp>
#include <ktl/move_only_function.hpp>
#include <misc/vec.hpp>
#include <vk/types.hpp>

namespace jk {
struct Swapchain {
	using GetExtent = ktl::move_only_function<UVec2()>;

	class Factory;

	struct Acquire {
		RenderImage image;
		std::uint32_t index{};
	};

	ktl::fixed_vector<RenderImage, 8> images;
	vk::SwapchainKHR swapchain;
	vk::SurfaceFormatKHR format;

	vk::ResultValue<Acquire> acquireNextImage(vk::Device device, vk::Semaphore signal);
	vk::Result present(Acquire image, vk::Semaphore wait, vk::Queue queue);
};

class Swapchain::Factory {
  public:
	Factory(GFX const& gfx, GetExtent&& getExtent) noexcept : m_extent(std::move(getExtent)), m_gfx(gfx) {}
	~Factory() { destroy(); }

	GFX const& gfx() const noexcept { return m_gfx; }
	Swapchain swapchain() const noexcept { return m_swapchain; }

	bool make();
	void destroy();

  private:
	struct Info {
		vk::Extent2D extent{};
		std::uint32_t imageCount{};
	};

	static Info makeInfo(GFX const& gfx, UVec2 extent);
	static vk::SurfaceFormatKHR makeSurfaceFormat(GFX const& gfx);
	static void destroy(GFX const& gfx, Swapchain& out);
	static vk::ImageView makeImageView(GFX const& gfx, vk::Image image, vk::Format format);

	GetExtent m_extent;
	GFX m_gfx;
	Swapchain m_swapchain;
	vk::SwapchainCreateInfoKHR createInfo;
};
} // namespace jk
