#include <app/jukebox.hpp>
#include <dibs/bridge.hpp>
#include <dibs/dibs.hpp>
#include <ktl/fixed_vector.hpp>
#include <misc/log.hpp>
#include <misc/version.hpp>

namespace {
std::string windowTitle(std::string_view appName) {
	if constexpr (jk_debug) {
		auto const app = jk::Version::app().toString(true);
		return ktl::kformat("{} {} | Debug", appName.data(), app.data());
	} else {
		auto const app = jk::Version::app().toString();
		return ktl::kformat("{} {}", appName.data(), app.data());
	}
}
} // namespace

int main() {
	auto file = jk::Log::toFile("jukebox_log.txt");
	auto dibsInst = dibs::Instance::Builder{}.extent({650U, 300U}).title(windowTitle("Jukebox").data()).flags(dibs::Instance::Flag::eHidden)();
	if (!dibsInst) { return 10; }
	auto jukebox = jk::Jukebox::make(dibs::Bridge::glfw(*dibsInst));
	if (!jukebox) { return 20; }
	glfwShowWindow(dibs::Bridge::glfw(*dibsInst));
	while (!dibsInst->closing()) {
		auto const poll = dibsInst->poll();
		for (auto const& ev : poll.events) {
			switch (ev.type()) {
			case dibs::Event::Type::eKey: jukebox->onKey(ev.key()); break;
			case dibs::Event::Type::eFileDrop: jukebox->onFileDrop(ev.fileDrop());
			default: break;
			}
		}
		auto frame = dibs::Frame(*dibsInst);
		jukebox->update();
	}
}
