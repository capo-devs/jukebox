#include <ktl/async_queue.hpp>
#include <misc/delta_time.hpp>
#include <misc/log.hpp>
#include <atomic>
#include <deque>
#include <fstream>
#include <iostream>
#include <thread>

namespace jk {
ktl::async_queue<std::string> g_queue;
std::atomic<bool> g_active;

struct Log::File::Impl {
	std::thread thread;

	Impl(std::string_view path) {
		if (!g_active.load()) {
			g_active.store(true);
			thread = std::thread([path]() {
				while (auto line = g_queue.pop()) {
					if (auto file = std::ofstream(path.data(), std::ios::app)) { file << *line; }
				}
			});
		}
	}

	~Impl() {
		if (thread.joinable()) {
			g_queue.active(false);
			thread.join();
			g_active.store(false);
		}
	}
};

Log::File::File(File&&) noexcept = default;
Log::File& Log::File::operator=(File&&) noexcept = default;
Log::File::~File() { stop(); }

Log::File::File(std::string_view path) noexcept {
	auto file = std::ofstream(path.data(), std::ios::out);
	if (file.good() && !g_active.load()) {
		file.close();
		debug("Logging to file [{}]", path);
		impl = std::make_unique<Impl>(path);
		if (!g_active.load()) {
			impl.reset();
			warn("File log [{}] failed", path);
		} else {
			info("jukebox [{}]", timeStr("%a %F %Z"));
		}
	}
}

bool Log::File::logging() const noexcept { return impl != nullptr; }

void Log::File::stop() {
	if (impl) { debug("Stopped logging to file"); }
	impl.reset();
}

void Log::print(Level level, std::string_view message) {
	static constexpr char levels[] = {'E', 'W', 'I', 'D'};
	std::ostream& out = level == Level::error ? std::cerr : std::cout;
	auto line = ktl::format("[{}] {} [{}]\n", levels[std::size_t(level)], message, timeStr());
	out << line;
	if (g_active.load()) { g_queue.push(std::move(line)); }
}
} // namespace jk
