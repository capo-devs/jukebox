#include <jk_version.hpp>
#include <ktl/async_queue.hpp>
#include <ktl/kthread.hpp>
#include <misc/delta_time.hpp>
#include <misc/log.hpp>
#include <fstream>
#include <iostream>

namespace jk {
struct FileLogger {
	ktl::async_queue<std::string> queue;
	ktl::kthread thread;
	std::string path;

	FileLogger(std::string p) : path(std::move(p)) {
		thread = ktl::kthread([this]() {
			while (auto line = queue.pop()) {
				if (auto file = std::ofstream(path, std::ios::app)) { file << *line; }
			}
		});
	}
};

std::optional<FileLogger> g_file;

Log::File::File(File&& rhs) noexcept { std::swap(active, rhs.active); }
Log::File& Log::File::operator=(File rhs) noexcept { return (std::swap(active, rhs.active), *this); }
Log::File::~File() noexcept {
	if (active && g_file) {
		g_file->queue.active(false);
		g_file.reset();
		info("Stopped file logging");
	}
}

std::optional<Log::File> Log::toFile(std::string path) {
	if (g_file) {
		print(Level::warn, ktl::format("Already logging to file [{}]", g_file->path), false);
		return std::nullopt;
	}
	if (auto file = std::ofstream(path); !file) {
		error("Failed to create log file [{}]", path);
		return std::nullopt;
	}
	g_file.emplace(std::move(path));
	print(Level::debug, ktl::format("Logging to file [{}]", g_file->path), false);
	print(Level::info, ktl::format("jukebox v{} | {}", jukebox_version, timeStr<stdch::system_clock>("%a %F (%Z)")));
	return File(true);
}

void Log::print(Level level, std::string_view message, bool file) {
	static constexpr char levels[] = {'E', 'W', 'I', 'D'};
	std::ostream& out = level == Level::error ? std::cerr : std::cout;
	auto line = ktl::format("[{}] {} [{}]\n", levels[std::size_t(level)], message, timeStr());
	out << line;
	if (file && g_file) { g_file->queue.push(std::move(line)); }
}
} // namespace jk
