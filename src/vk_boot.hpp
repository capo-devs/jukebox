#pragma once
#include <vulkan/vulkan.hpp>
#include <limits>
#include <optional>

namespace jk {
template <typename D>
concept vk_dispatch = std::is_same_v<D, vk::Instance> || std::is_same_v<D, vk::Device>;

constexpr auto limitless_extent_v = std::numeric_limits<std::uint32_t>::max();
constexpr auto limitless_timeout_v = std::numeric_limits<std::uint64_t>::max();

template <typename... T>
constexpr bool allValid(T... t) noexcept {
	return ((t != T{}) && ...);
}

template <typename T, typename... Ts>
constexpr bool anyOf(T t, Ts... ts) noexcept {
	return ((t == ts) || ...);
}

///
/// \brief RAII vk::T handle, owned by Dispatch (or itself if void)
///
/// calls Dispatch.destroy(T) / T.destroy() on destruction
///
template <typename T, typename Dispatch = vk::Device>
class Box;

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

  private:
	static void exchg(Box& lhs, Box& rhs) noexcept {
		std::swap(lhs.m_t, rhs.m_t);
		std::swap(lhs.m_dispatch, rhs.m_dispatch);
	}

	T m_t;
	Dispatch m_dispatch;
};

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

	bool valid() const noexcept { return allValid(instance, surface, physicalDevice, device, queue.queue); }
};

struct SurfaceMaker;

class VkBoot {
  public:
	static std::optional<VkBoot> make(SurfaceMaker const& surfaceMaker) {
		VkBoot ret;
		if (ret.tryMake(surfaceMaker)) { return ret; }
		return std::nullopt;
	}

	GFX const& gfx() const noexcept { return m_gfx; }

  private:
	bool tryMake(SurfaceMaker const& surfaceMaker);

	GFX m_gfx;
	Box<vk::Instance, void> m_instance;
	Box<vk::DebugUtilsMessengerEXT, vk::Instance> m_messenger;
	Box<vk::SurfaceKHR, vk::Instance> m_surface;
	Box<vk::Device, void> m_device;
};
} // namespace jk
