#include <imgui_instance.hpp>
#include <ktl/fixed_vector.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <iostream>

#include <GLFW/glfw3.h>

extern void ImGui_ImplVulkanH_DestroyFrame(VkDevice device, ImGui_ImplVulkanH_Frame* fd, const VkAllocationCallbacks* allocator);
extern void ImGui_ImplVulkanH_DestroyFrameSemaphores(VkDevice device, ImGui_ImplVulkanH_FrameSemaphores* fsd, const VkAllocationCallbacks* allocator);

namespace jk {
namespace {
bool g_SwapChainRebuild{};

constexpr vk::Extent2D clamp(vk::Extent2D target, vk::Extent2D lo, vk::Extent2D hi) noexcept {
	auto const x = std::clamp(target.width, lo.width, hi.width);
	auto const y = std::clamp(target.height, lo.height, hi.height);
	return {x, y};
}

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
	vk::CommandBuffer cb;
};

struct RenderSync {
	struct Sync {
		RenderFrame frame;
		vk::Framebuffer framebuffer;
		vk::CommandPool pool;
		vk::Semaphore draw;
		vk::Semaphore present;
		vk::Fence drawn;
	};

	Sync sync[2] = {};
	std::size_t index{};

	Sync& get() noexcept { return sync[index]; }
	void next() noexcept { index = (index + 1) % 2; }
};

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

class Swapchain::Factory {
  public:
	Factory(GLFWwindow* window) noexcept : m_window(window) {}

	Swapchain swapchain() const noexcept { return m_swapchain; }

	Swapchain make(GFX const& gfx);
	void destroy(GFX const& gfx);

  private:
	struct Info {
		vk::Extent2D extent{};
		std::uint32_t imageCount{};
	};

	static Info makeInfo(GFX const& gfx, GLFWwindow* window);
	static vk::SurfaceFormatKHR makeSurfaceFormat(GFX const& gfx);
	static void destroy(GFX const& gfx, Swapchain& out);
	static vk::ImageView makeImageView(GFX const& gfx, vk::Image image, vk::Format format);

	Swapchain m_swapchain;
	vk::SwapchainCreateInfoKHR createInfo;
	GLFWwindow* m_window{};
};

Swapchain Swapchain::Factory::make(GFX const& gfx) {
	createInfo.oldSwapchain = m_swapchain.swapchain;
	createInfo.presentMode = vk::PresentModeKHR::eFifo;
	createInfo.imageArrayLayers = 1U;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	createInfo.pQueueFamilyIndices = &gfx.queue.family;
	createInfo.queueFamilyIndexCount = 1U;
	createInfo.surface = gfx.surface;
	m_swapchain.format = makeSurfaceFormat(gfx);
	createInfo.imageFormat = m_swapchain.format.format;
	createInfo.imageColorSpace = m_swapchain.format.colorSpace;
	gfx.device.waitIdle();
	auto const info = makeInfo(gfx, m_window);
	createInfo.imageExtent = info.extent;
	createInfo.minImageCount = info.imageCount;
	Swapchain old = std::move(m_swapchain);
	gfx.device.createSwapchainKHR(&createInfo, nullptr, &m_swapchain.swapchain);
	destroy(gfx, old);
	auto const images = gfx.device.getSwapchainImagesKHR(m_swapchain.swapchain);
	for (auto const image : images) { m_swapchain.images.push_back({info.extent, image, makeImageView(gfx, image, createInfo.imageFormat)}); }
	return m_swapchain;
}

void Swapchain::Factory::destroy(GFX const& gfx) { destroy(gfx, m_swapchain); }

Swapchain::Factory::Info Swapchain::Factory::makeInfo(GFX const& gfx, GLFWwindow* window) {
	Info ret;
	auto const caps = gfx.physicalDevice.getSurfaceCapabilitiesKHR(gfx.surface);
	if (caps.currentExtent != limitless_extent_v) {
		ret.extent = caps.currentExtent;
	} else {
		ret.extent = clamp(ImGuiInstance::framebufferSize(window), caps.minImageExtent, caps.maxImageExtent);
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

class Renderer {
  public:
	using Clear = std::array<float, 4>;

	Renderer(GFX const& gfx, GLFWwindow* window);
	~Renderer();

	std::optional<RenderFrame> nextFrame(Clear const& clear);
	bool submit();

	vk::RenderPass renderPass() const noexcept { return m_pass; }
	std::uint32_t imageCount() const noexcept { return std::uint32_t(m_factory.swapchain().images.size()); }

	static vk::CommandBuffer makeCB(vk::Device device, vk::CommandPool pool);

  private:
	static vk::Framebuffer makeFramebuffer(vk::Device device, vk::RenderPass pass, RenderImage const& image);
	static vk::RenderPass makeRenderPass(vk::Device device, vk::Format colour);

	static void cycleFramebuffer(vk::Framebuffer& out, vk::Device device, vk::RenderPass pass, RenderImage const& image);
	static void cycleFence(vk::Fence& out, vk::Device device);

	bool swapchainCheck(vk::Result result);

	GFX m_gfx;
	Swapchain::Factory m_factory;
	RenderSync m_sync;
	vk::RenderPass m_pass;
	std::optional<Swapchain::Acquire> m_acquired;
};

Renderer::Renderer(GFX const& gfx, GLFWwindow* window) : m_gfx(gfx), m_factory(window) {
	m_factory.make(m_gfx);
	for (auto& sync : m_sync.sync) {
		sync.drawn = m_gfx.device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
		sync.draw = m_gfx.device.createSemaphore({});
		sync.present = m_gfx.device.createSemaphore({});
		sync.pool = m_gfx.device.createCommandPool(vk::CommandPoolCreateInfo({}, gfx.queue.family));
		sync.frame.cb = makeCB(m_gfx.device, sync.pool);
	}
	m_pass = makeRenderPass(m_gfx.device, m_factory.swapchain().format.format);
}

Renderer::~Renderer() {
	m_gfx.device.waitIdle();
	for (auto const& sync : m_sync.sync) {
		m_gfx.device.destroyFence(sync.drawn);
		m_gfx.device.destroySemaphore(sync.draw);
		m_gfx.device.destroySemaphore(sync.present);
		m_gfx.device.destroyCommandPool(sync.pool);
		if (allValid(sync.framebuffer)) { m_gfx.device.destroyFramebuffer(sync.framebuffer); }
	}
	m_gfx.device.destroyRenderPass(m_pass);
	m_factory.destroy(m_gfx);
}

std::optional<RenderFrame> Renderer::nextFrame(Clear const& clear) {
	auto& sync = m_sync.get();
	cycleFence(sync.drawn, m_gfx.device);
	auto acquire = m_factory.swapchain().acquireNextImage(m_gfx.device, sync.draw);
	if (!swapchainCheck(acquire.result)) { return std::nullopt; }
	m_acquired = acquire.value;
	sync.frame.target.colour = acquire.value.image;
	cycleFramebuffer(sync.framebuffer, m_gfx.device, m_pass, sync.frame.target.colour);
	m_gfx.device.resetCommandPool(sync.pool, {});
	sync.frame.cb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	vk::ClearValue cv = vk::ClearColorValue(clear);
	vk::RenderPassBeginInfo info;
	info.renderPass = m_pass;
	info.framebuffer = sync.framebuffer;
	info.renderArea.extent.width = sync.frame.target.colour.extent.width;
	info.renderArea.extent.height = sync.frame.target.colour.extent.height;
	info.clearValueCount = 1;
	info.pClearValues = &cv;
	sync.frame.cb.beginRenderPass(info, vk::SubpassContents::eInline);
	return sync.frame;
}

bool Renderer::submit() {
	if (!m_acquired) { return false; }
	auto& sync = m_sync.get();
	sync.frame.cb.endRenderPass();
	vk::PipelineStageFlags wait = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo info;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &sync.draw;
	info.pWaitDstStageMask = &wait;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &sync.frame.cb;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = &sync.present;
	sync.frame.cb.end();
	m_gfx.queue.queue.submit(info, sync.drawn);
	if (!swapchainCheck(m_factory.swapchain().present(*m_acquired, sync.present, m_gfx.queue.queue))) { return false; }
	m_sync.next();
	return true;
}

vk::Framebuffer Renderer::makeFramebuffer(vk::Device device, vk::RenderPass pass, RenderImage const& image) {
	vk::FramebufferCreateInfo info;
	info.renderPass = pass;
	info.attachmentCount = 1;
	info.pAttachments = &image.view;
	info.width = image.extent.width;
	info.height = image.extent.height;
	info.layers = 1;
	return device.createFramebuffer(info);
}

vk::RenderPass Renderer::makeRenderPass(vk::Device device, vk::Format colour) {
	vk::AttachmentDescription attachment;
	attachment.format = colour;
	attachment.samples = vk::SampleCountFlagBits::e1;
	attachment.loadOp = vk::AttachmentLoadOp::eClear;
	attachment.storeOp = vk::AttachmentStoreOp::eStore;
	attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachment.initialLayout = vk::ImageLayout::eUndefined;
	attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
	vk::AttachmentReference color_attachment;
	color_attachment.attachment = 0;
	color_attachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	vk::RenderPassCreateInfo info;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;
	return device.createRenderPass(info);
}

vk::CommandBuffer Renderer::makeCB(vk::Device device, vk::CommandPool pool) {
	vk::CommandBufferAllocateInfo info;
	info.commandBufferCount = 1;
	info.commandPool = pool;
	return device.allocateCommandBuffers(info).front();
}

void Renderer::cycleFramebuffer(vk::Framebuffer& out, vk::Device device, vk::RenderPass pass, RenderImage const& image) {
	if (allValid(out)) { device.destroyFramebuffer(out); }
	out = makeFramebuffer(device, pass, image);
}

void Renderer::cycleFence(vk::Fence& out, vk::Device device) {
	if (allValid(out)) { device.waitForFences(out, true, limitless_timeout_v); }
	device.resetFences(out);
}

bool Renderer::swapchainCheck(vk::Result ret) {
	if (ret != vk::Result::eSuccess) {
		if (anyOf(ret, vk::Result::eSuboptimalKHR, vk::Result::eErrorOutOfDateKHR)) { m_factory.make(m_gfx); }
		return false;
	}
	return true;
}

void initImGui(GFX const& gfx, GLFWwindow* window, vk::DescriptorPool pool, vk::RenderPass pass, std::uint32_t imageCount) noexcept {
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = gfx.instance;
	init_info.PhysicalDevice = gfx.physicalDevice;
	init_info.Device = gfx.device;
	init_info.QueueFamily = gfx.queue.family;
	init_info.Queue = gfx.queue.queue;
	// init_info.PipelineCache = g_PipelineCache;
	init_info.DescriptorPool = pool;
	// init_info.Allocator = g_Allocator;
	init_info.MinImageCount = 3;
	init_info.ImageCount = imageCount;
	// init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, pass);
}

vk::DescriptorPool makePool(GFX const& gfx) {
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
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	return gfx.device.createDescriptorPool(pool_info);
}

void uploadFonts(GFX const& gfx) {
	auto pool = gfx.device.createCommandPool(vk::CommandPoolCreateInfo({}, gfx.queue.family));
	auto cb = Renderer::makeCB(gfx.device, pool);
	cb.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	ImGui_ImplVulkan_CreateFontsTexture(cb);
	cb.end();
	vk::SubmitInfo end_info;
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &cb;
	gfx.queue.queue.submit(end_info, {});
	gfx.device.waitIdle();
	gfx.device.destroyCommandPool(pool);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

GLFWwindowiconifyfun g_onIconify{};
void onIconify(GLFWwindow* window, int iconified) noexcept {
	if (iconified == 0) { g_SwapChainRebuild = true; }
	if (g_onIconify) { g_onIconify(window, iconified); }
}

// TODO: remove
std::optional<Renderer> g_r;
} // namespace

std::optional<ImGuiInstance> ImGuiInstance::make(GFX const& gfx, GLFWwindow* window) {
	g_r.emplace(gfx, window);
	g_onIconify = glfwSetWindowIconifyCallback(window, &onIconify);
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	static vk::Instance s_inst;
	static vk::DynamicLoader s_dl;
	s_inst = gfx.instance;
	auto loader = [](char const* fn, void*) { return s_dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")(s_inst, fn); };
	ImGui_ImplVulkan_LoadFunctions(loader);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiInstance ret;
	ret.m_pool = makePool(gfx);
	initImGui(gfx, window, ret.m_pool, g_r->renderPass(), g_r->imageCount());
	ret.m_init = true;
	ret.m_gfx = gfx;
	uploadFonts(gfx);
	return ret;
}

ImGuiInstance::~ImGuiInstance() noexcept {
	if (m_init) {
		assert(m_gfx.valid());
		m_gfx.device.waitIdle();
		g_r.reset();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		if (allValid(m_pool)) { m_gfx.device.destroy(m_pool); }
	}
}

ImGuiFrame ImGuiInstance::frame() { return {}; }

vk::Extent2D ImGuiInstance::framebufferSize(GLFWwindow* window) {
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	return vk::Extent2D(std::uint32_t(w), std::uint32_t(h));
}

vk::Extent2D ImGuiInstance::windowSize(GLFWwindow* window) {
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	return vk::Extent2D(std::uint32_t(w), std::uint32_t(h));
}

void ImGuiInstance::exchg(ImGuiInstance& lhs, ImGuiInstance& rhs) noexcept {
	std::swap(lhs.m_init, rhs.m_init);
	std::swap(lhs.m_gfx, rhs.m_gfx);
	std::swap(lhs.m_pool, rhs.m_pool);
}

ImGuiFrame::ImGuiFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

ImGuiFrame::~ImGuiFrame() {
	ImGui::Render();

	if (auto frame = g_r->nextFrame({})) {
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame->cb);
		g_r->submit();
	}

	/*if (!g_SwapChainRebuild && g_MainWindowData.Width > 0 && g_MainWindowData.Height > 0) {
		TEMP_FrameRender(m_gfx, &g_MainWindowData, ImGui::GetDrawData());
		TEMP_FramePresent(m_gfx, &g_MainWindowData);
	}
	if (g_SwapChainRebuild) {
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		if (width > 0 && height > 0) {
			ImGui_ImplVulkanH_CreateOrResizeWindow(m_gfx.instance, m_gfx.physicalDevice, m_gfx.device, &g_MainWindowData, m_gfx.queue.family, nullptr, width,
												   height, 3);
			g_MainWindowData.FrameIndex = 0;
			g_SwapChainRebuild = false;
		}
	}*/
}
} // namespace jk
