#pragma once
#include <ktl/fixed_vector.hpp>
#include <vk/types.hpp>

struct GLFWwindow;

namespace jk {
struct Swapchain {
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
	Factory(GFX const& gfx, GLFWwindow* window) noexcept : m_gfx(gfx), m_window(window) {}
	~Factory() { destroy(); }

	GFX const& gfx() const noexcept { return m_gfx; }
	GLFWwindow* window() const noexcept { return m_window; }
	Swapchain swapchain() const noexcept { return m_swapchain; }

	bool make();
	void destroy();

  private:
	struct Info {
		vk::Extent2D extent{};
		std::uint32_t imageCount{};
	};

	static Info makeInfo(GFX const& gfx, GLFWwindow* window);
	static vk::SurfaceFormatKHR makeSurfaceFormat(GFX const& gfx);
	static void destroy(GFX const& gfx, Swapchain& out);
	static vk::ImageView makeImageView(GFX const& gfx, vk::Image image, vk::Format format);

	GFX m_gfx;
	Swapchain m_swapchain;
	vk::SwapchainCreateInfoKHR createInfo;
	GLFWwindow* m_window{};
};
} // namespace jk
