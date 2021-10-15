#pragma once
#include <jk_common.hpp>
#include <vulkan/vulkan.hpp>
#include <limits>

namespace jk {
template <typename D>
concept vk_dispatch = std::is_same_v<D, vk::Instance> || std::is_same_v<D, vk::Device>;

constexpr auto limitless_timeout_v = std::numeric_limits<std::uint64_t>::max();

constexpr bool limitlessExtent(vk::Extent2D extent) noexcept;

///
/// \brief RAII vk::T handle, owned by Dispatch (or itself if void)
///
/// calls Dispatch.destroy(T) / T.destroy() on destruction
///
template <typename T, typename Dispatch = vk::Device>
class Box;

vk::CommandBuffer makeCommandBuffer(vk::Device device, vk::CommandPool pool);
Box<vk::Framebuffer> makeFramebuffer(vk::Device device, vk::RenderPass pass, struct RenderImage const& image);

///
/// \brief Rotating ring buffer of Ts
///
template <typename T, std::size_t N>
struct RingBuffer;

struct Queue {
	vk::Queue queue;
	std::uint32_t family{};
};

struct GFX {
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	Queue queue;
	vk::SurfaceKHR surface;
};

struct RenderImage {
	vk::Extent2D extent;
	vk::Image image;
	vk::ImageView view;
};

struct RenderTarget {
	RenderImage colour;
};

struct RenderFrame {
	RenderTarget target;
	vk::CommandBuffer cmd;
};

struct SurfaceMaker {
	virtual vk::SurfaceKHR operator()(vk::Instance instance) const = 0;
};

template <typename T>
class Box<T, void> {
  public:
	explicit Box(T t = {}) noexcept : m_t(t) {}
	Box(Box&& rhs) noexcept : Box() { std::swap(m_t, rhs.m_t); }
	Box& operator=(Box rhs) noexcept { return (std::swap(m_t, rhs.m_t), *this); }
	~Box() noexcept {
		if (allValid(m_t)) { m_t.destroy(); }
	}

	T get() const noexcept { return m_t; }
	operator T() const noexcept { return get(); }

  private:
	T m_t;
};

template <typename T, typename Dispatch>
	requires vk_dispatch<Dispatch>
class Box<T, Dispatch> {
  public:
	Box() = default;
	Box(T t, Dispatch dispatch) noexcept : m_t(t), m_dispatch(dispatch) {}
	Box(Box&& rhs) noexcept : Box() { exchg(*this, rhs); }
	Box& operator=(Box rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Box() noexcept {
		if (allValid(m_t, m_dispatch)) { m_dispatch.destroy(m_t); }
	}

	T get() const noexcept { return m_t; }
	operator T() const noexcept { return get(); }

	Dispatch dispatch() const noexcept { return m_dispatch; }

  private:
	static void exchg(Box& lhs, Box& rhs) noexcept {
		std::swap(lhs.m_t, rhs.m_t);
		std::swap(lhs.m_dispatch, rhs.m_dispatch);
	}

	T m_t;
	Dispatch m_dispatch;
};

template <typename T, std::size_t Count = 2>
struct RingBuffer {
	T ts[Count] = {};
	std::size_t index{};

	constexpr T& get() noexcept { return ts[index]; }
	constexpr void next() noexcept { index = (index + 1) % Count; }
};

// impl

constexpr bool limitlessExtent(vk::Extent2D extent) noexcept {
	auto const value = std::numeric_limits<std::uint32_t>::max();
	return extent.width == value && extent.height == value;
}

inline vk::CommandBuffer makeCommandBuffer(vk::Device device, vk::CommandPool pool) {
	vk::CommandBufferAllocateInfo info;
	info.commandBufferCount = 1;
	info.commandPool = pool;
	return device.allocateCommandBuffers(info).front();
}

inline Box<vk::Framebuffer> makeFramebuffer(vk::Device device, vk::RenderPass pass, RenderImage const& image) {
	vk::FramebufferCreateInfo info;
	info.renderPass = pass;
	info.attachmentCount = 1;
	info.pAttachments = &image.view;
	info.width = image.extent.width;
	info.height = image.extent.height;
	info.layers = 1;
	return {device.createFramebuffer(info), device};
}
} // namespace jk
