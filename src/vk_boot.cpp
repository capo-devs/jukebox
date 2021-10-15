#include <common.hpp>
#include <vk_boot.hpp>
#include <vk_surface_maker.hpp>
#include <VkBootstrap.h>
#include <iostream>

namespace jk {
bool VkBoot::tryMake(SurfaceMaker const& surfaceMaker) {
	vkb::InstanceBuilder builder;
	auto inst_ret = builder.set_app_name("Jukebox").request_validation_layers(jk_debug).use_default_debug_messenger().build();
	if (!inst_ret) {
		std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
		return false;
	}
	vkb::Instance vkb_inst = inst_ret.value();
	auto instance = Box<vk::Instance, void>(vkb_inst.instance);
	auto messenger = Box<vk::DebugUtilsMessengerEXT, vk::Instance>(inst_ret->debug_messenger, instance);
	vkb::PhysicalDeviceSelector selector{vkb_inst};
	vk::SurfaceKHR vk_surface = surfaceMaker(instance);
	if (!allValid(vk_surface)) {
		std::cerr << "Failed to create Vulkan Surface\n";
		return false;
	}
	auto surface = Box<vk::SurfaceKHR, vk::Instance>(vk_surface, instance);
	auto phys_ret = selector.set_surface(vk_surface).set_minimum_version(1, 1).select();
	if (!phys_ret) {
		std::cerr << "Failed to select Vulkan Physical Device. Error: " << phys_ret.error().message() << "\n";
		return false;
	}
	std::cout << "Using [" << phys_ret->properties.deviceName << "]\n";
	vkb::DeviceBuilder device_builder{phys_ret.value()};
	auto dev_ret = device_builder.build();
	if (!dev_ret) {
		std::cerr << "Failed to create Vulkan device. Error: " << dev_ret.error().message() << "\n";
		return false;
	}
	auto device = Box<vk::Device, void>(dev_ret->device);
	vkb::Device vkb_device = dev_ret.value();
	auto graphics_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
	if (!graphics_queue_ret) {
		std::cerr << "Failed to get graphics queue. Error: " << graphics_queue_ret.error().message() << "\n";
		return false;
	}
	auto const family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
	m_instance = std::move(instance);
	m_messenger = std::move(messenger);
	m_device = std::move(device);
	m_surface = std::move(surface);
	m_gfx = {m_instance, dev_ret->physical_device.physical_device, m_device, {graphics_queue_ret.value(), family}, m_surface};
	vk::DynamicLoader dl;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_gfx.instance, dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"), m_gfx.device);
	return true;
}
} // namespace jk
