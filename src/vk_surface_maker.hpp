#pragma once

namespace vk {
class SurfaceKHR;
class Instance;
} // namespace vk

namespace jk {
struct SurfaceMaker {
	virtual vk::SurfaceKHR operator()(vk::Instance instance) const = 0;
};
} // namespace jk
