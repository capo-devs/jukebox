#pragma once
#include <jk_common.hpp>
#include <vk/types.hpp>
#include <optional>

namespace jk {
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
