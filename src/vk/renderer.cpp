#include <jk_common.hpp>
#include <misc/log.hpp>
#include <vk/renderer.hpp>
#include <thread>

namespace jk {
Renderer::Renderer(GFX const& gfx, Swapchain::GetExtent&& getExtent, IconifySignal&& onIconify)
	: m_factory(gfx, std::move(getExtent)), m_onIconify(std::move(onIconify)) {
	m_onIconify += [this](bool iconified) { this->onIconify(iconified); };
	m_factory.make();
	for (auto& sync : m_sync.ts) {
		sync.pool = {gfx.device.createCommandPool(vk::CommandPoolCreateInfo({}, gfx.queue.family)), gfx.device};
		sync.frame.cmd = makeCommandBuffer(gfx.device, sync.pool);
	}
	resync();
	m_pass = makeRenderPass(gfx.device, m_factory.swapchain().format.format);
	Log::debug("Swapchain Renderer constructed, image count: [{}], buffering: [{}]", imageCount(), buffering_v);
}

std::optional<RenderFrame> Renderer::nextFrame(Clear const& clear) {
	if (paused()) { return std::nullopt; }
	auto& sync = m_sync.get();
	cycleFence(sync.drawn);
	auto acquire = m_factory.swapchain().acquireNextImage(m_factory.gfx().device, sync.draw);
	if (!swapchainCheck(acquire.result)) { return std::nullopt; }
	m_acquired = acquire.value;
	sync.frame.target.colour = acquire.value.image;
	cycleFramebuffer(sync.framebuffer, sync.frame.target.colour);
	m_factory.gfx().device.resetCommandPool(sync.pool, {});
	sync.frame.cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	vk::ClearValue cv = vk::ClearColorValue(clear);
	vk::RenderPassBeginInfo info;
	info.renderPass = m_pass;
	info.framebuffer = sync.framebuffer;
	info.renderArea.extent.width = sync.frame.target.colour.extent.width;
	info.renderArea.extent.height = sync.frame.target.colour.extent.height;
	info.clearValueCount = 1;
	info.pClearValues = &cv;
	sync.frame.cmd.beginRenderPass(info, vk::SubpassContents::eInline);
	return sync.frame;
}

bool Renderer::present(RenderFrame const& frame) {
	if (!m_acquired || frame.target.colour.image != m_acquired->image.image || paused()) { return false; }
	auto& sync = m_sync.get();
	sync.frame.cmd.endRenderPass();
	vk::PipelineStageFlags wait = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo info;
	auto const draw = sync.draw.get();
	auto const present = sync.present.get();
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &draw;
	info.pWaitDstStageMask = &wait;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &sync.frame.cmd;
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = &present;
	sync.frame.cmd.end();
	m_factory.gfx().queue.queue.submit(info, sync.drawn);
	if (!swapchainCheck(m_factory.swapchain().present(*m_acquired, sync.present, m_factory.gfx().queue.queue))) { return false; }
	m_sync.next();
	return true;
}

void Renderer::onIconify(bool iconified) {
	if (iconified) {
		m_flags.set(Flag::ePaused);
		Log::debug("Window iconified, pausing rendering");
	} else {
		m_flags = Flag::eRebuild;
		Log::debug("Window uniconfied, swapchain marked for rebuild");
	}
}

Box<vk::RenderPass> Renderer::makeRenderPass(vk::Device device, vk::Format colour) {
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
	return {device.createRenderPass(info), device};
}

void Renderer::cycleFramebuffer(Box<vk::Framebuffer>& out, RenderImage const& image) { out = makeFramebuffer(m_factory.gfx().device, m_pass, image); }

void Renderer::cycleFence(Box<vk::Fence>& out) {
	out.dispatch().waitForFences(out.get(), true, limitless_timeout_v);
	out.dispatch().resetFences(out.get());
}

bool Renderer::swapchainCheck(vk::Result ret) {
	if (m_flags.test(Flag::ePaused)) { return false; }
	if (anyOf(ret, vk::Result::eSuboptimalKHR, vk::Result::eErrorOutOfDateKHR) || m_flags.test(Flag::eRebuild)) {
		static constexpr int max_attempts_v = 5;
		int fails = 0;
		bool recreateSuccess = false;
		while (!recreateSuccess && fails < max_attempts_v) {
			if (recreateSuccess = m_factory.make(); !recreateSuccess) {
				std::this_thread::yield();
				++fails;
			}
		}
		assert(recreateSuccess);
		m_flags.reset(Flag::eRebuild);
		resync();
		return false;
	}
	return ret == vk::Result::eSuccess;
}

void Renderer::resync() {
	auto const& gfx = m_factory.gfx();
	for (auto& sync : m_sync.ts) {
		sync.drawn = {gfx.device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)), gfx.device};
		sync.draw = {gfx.device.createSemaphore({}), gfx.device};
		sync.present = {gfx.device.createSemaphore({}), gfx.device};
		gfx.device.resetCommandPool(sync.pool, {});
	}
}

bool Renderer::paused() const noexcept { return m_flags.test(Flag::ePaused); }
} // namespace jk
