#include <gui/imgui_instance.hpp>
#include <jk_common.hpp>
#include <win/glfw_instance.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <iostream>

#include <GLFW/glfw3.h>

namespace jk {
namespace {
void initImGui(GFX const& gfx, GLFWwindow* window, vk::DescriptorPool pool, vk::RenderPass pass, std::uint32_t imageCount) noexcept {
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = gfx.instance;
	init_info.PhysicalDevice = gfx.physicalDevice;
	init_info.Device = gfx.device;
	init_info.QueueFamily = gfx.queue.family;
	init_info.Queue = gfx.queue.queue;
	init_info.DescriptorPool = pool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = imageCount;
	ImGui_ImplVulkan_Init(&init_info, pass);
}

Box<vk::DescriptorPool> makePool(GFX const& gfx) {
	vk::DescriptorPoolSize pool_sizes[] = {
		{vk::DescriptorType::eSampler, 1000},
		{vk::DescriptorType::eCombinedImageSampler, 1000},
		{vk::DescriptorType::eSampledImage, 1000},
		{vk::DescriptorType::eStorageImage, 1000},
		{vk::DescriptorType::eUniformTexelBuffer, 1000},
		{vk::DescriptorType::eStorageTexelBuffer, 1000},
		{vk::DescriptorType::eUniformBuffer, 1000},
		{vk::DescriptorType::eStorageBuffer, 1000},
		{vk::DescriptorType::eUniformBufferDynamic, 1000},
		{vk::DescriptorType::eStorageBufferDynamic, 1000},
		{vk::DescriptorType::eInputAttachment, 1000},
	};
	vk::DescriptorPoolCreateInfo pool_info;
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (std::uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	return {gfx.device.createDescriptorPool(pool_info), gfx.device};
}

void uploadFonts(GFX const& gfx) {
	auto pool = gfx.device.createCommandPool(vk::CommandPoolCreateInfo({}, gfx.queue.family));
	auto cmd = makeCommandBuffer(gfx.device, pool);
	cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	ImGui_ImplVulkan_CreateFontsTexture(cmd);
	cmd.end();
	vk::SubmitInfo end_info;
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &cmd;
	gfx.queue.queue.submit(end_info, {});
	gfx.device.waitIdle();
	gfx.device.destroyCommandPool(pool);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}
} // namespace

std::optional<ImGuiInstance> ImGuiInstance::make(GFX const& gfx, GLFWwindow* window, vk::RenderPass pass, std::uint32_t imageCount) {
	static vk::Instance s_inst;
	static vk::DynamicLoader const s_dl;
	s_inst = gfx.instance;
	auto const loader = [](char const* fn, void*) { return s_dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")(s_inst, fn); };
	ImGui_ImplVulkan_LoadFunctions(loader);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiInstance ret;
	ret.m_pool = makePool(gfx);
	ret.m_device = gfx.device;
	initImGui(gfx, window, ret.m_pool, pass, imageCount);
	uploadFonts(gfx);
	ret.m_init = true;
	return ret;
}

ImGuiInstance::~ImGuiInstance() noexcept {
	if (m_init) {
		m_device.waitIdle();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}

void ImGuiInstance::beginFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiInstance::render(vk::CommandBuffer cb) {
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
}

void ImGuiInstance::exchg(ImGuiInstance& lhs, ImGuiInstance& rhs) noexcept {
	std::swap(lhs.m_init, rhs.m_init);
	std::swap(lhs.m_pool, rhs.m_pool);
	std::swap(lhs.m_device, rhs.m_device);
}
} // namespace jk
