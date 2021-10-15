#include <jk_common.hpp>
#include <vk/swapchain.hpp>
#include <win/glfw_instance.hpp>

namespace jk {
namespace {
constexpr vk::Extent2D clamp(UVec2 target, vk::Extent2D lo, vk::Extent2D hi) noexcept {
	auto const x = std::clamp(target.x, lo.width, hi.width);
	auto const y = std::clamp(target.y, lo.height, hi.height);
	return {x, y};
}
} // namespace

vk::ResultValue<Swapchain::Acquire> Swapchain::acquireNextImage(vk::Device device, vk::Semaphore signal) {
	std::uint32_t idx{};
	auto const ret = device.acquireNextImageKHR(swapchain, limitless_timeout_v, signal, {}, &idx);
	if (ret != vk::Result::eSuccess) { return {ret, {}}; }
	auto const i = std::size_t(idx);
	assert(i < images.size());
	return {ret, {images[i], idx}};
}

vk::Result Swapchain::present(Acquire image, vk::Semaphore wait, vk::Queue queue) {
	vk::PresentInfoKHR info;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &wait;
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain;
	info.pImageIndices = &image.index;
	return queue.presentKHR(&info);
}

bool Swapchain::Factory::make() {
	createInfo.oldSwapchain = m_swapchain.swapchain;
	createInfo.presentMode = vk::PresentModeKHR::eFifo;
	createInfo.imageArrayLayers = 1U;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	createInfo.pQueueFamilyIndices = &m_gfx.queue.family;
	createInfo.queueFamilyIndexCount = 1U;
	createInfo.surface = m_gfx.surface;
	m_swapchain.format = makeSurfaceFormat(m_gfx);
	createInfo.imageFormat = m_swapchain.format.format;
	createInfo.imageColorSpace = m_swapchain.format.colorSpace;
	m_gfx.device.waitIdle();
	auto const info = makeInfo(m_gfx, m_window);
	if (info.extent.width == 0 || info.extent.height == 0) { return false; }
	createInfo.imageExtent = info.extent;
	createInfo.minImageCount = info.imageCount;
	Swapchain old = std::move(m_swapchain);
	auto const ret = m_gfx.device.createSwapchainKHR(&createInfo, nullptr, &m_swapchain.swapchain) == vk::Result::eSuccess;
	destroy(m_gfx, old);
	if (ret) {
		auto const images = m_gfx.device.getSwapchainImagesKHR(m_swapchain.swapchain);
		for (auto const image : images) { m_swapchain.images.push_back({info.extent, image, makeImageView(m_gfx, image, createInfo.imageFormat)}); }
	}
	return ret;
}

void Swapchain::Factory::destroy() { destroy(m_gfx, m_swapchain); }

Swapchain::Factory::Info Swapchain::Factory::makeInfo(GFX const& gfx, GLFWwindow* window) {
	Info ret;
	auto const caps = gfx.physicalDevice.getSurfaceCapabilitiesKHR(gfx.surface);
	if (!limitlessExtent(caps.currentExtent)) {
		ret.extent = caps.currentExtent;
	} else {
		ret.extent = clamp(framebufferSize(window), caps.minImageExtent, std::max(caps.maxImageExtent, caps.minImageExtent));
	}
	ret.imageCount = std::clamp(3U, caps.minImageCount, caps.maxImageCount);
	return ret;
}

vk::SurfaceFormatKHR Swapchain::Factory::makeSurfaceFormat(GFX const& gfx) {
	auto const formats = gfx.physicalDevice.getSurfaceFormatsKHR(gfx.surface);
	for (auto const& format : formats) {
		if (format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			if (anyOf(format.format, vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm)) { return format; }
		}
	}
	return formats.empty() ? vk::SurfaceFormatKHR() : formats.front();
}

vk::ImageView Swapchain::Factory::makeImageView(GFX const& gfx, vk::Image image, vk::Format format) {
	vk::ImageViewCreateInfo info;
	info.viewType = vk::ImageViewType::e2D;
	info.format = format;
	info.components.r = vk::ComponentSwizzle::eR;
	info.components.g = vk::ComponentSwizzle::eG;
	info.components.b = vk::ComponentSwizzle::eB;
	info.components.a = vk::ComponentSwizzle::eA;
	info.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
	info.image = image;
	return gfx.device.createImageView(info);
}

void Swapchain::Factory::destroy(GFX const& gfx, Swapchain& out) {
	if (allValid(out.swapchain)) {
		gfx.device.waitIdle();
		for (auto const& image : out.images) {
			if (allValid(image.view)) { gfx.device.destroyImageView(image.view); }
		}
		gfx.device.destroySwapchainKHR(out.swapchain);
	}
	out.images.clear();
	out.swapchain = vk::SwapchainKHR();
}
} // namespace jk
