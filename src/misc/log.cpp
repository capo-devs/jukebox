#include <jk_version.hpp>
#include <ktl/async/async_queue.hpp>
#include <ktl/async/kthread.hpp>
#include <misc/log.hpp>
#include <chrono>
#include <fstream>
#include <iostream>

namespace jk {
namespace stdch = std::chrono;

namespace {
std::string timeStr(std::string_view fmt = "%T", stdch::system_clock::time_point const& stamp = stdch::system_clock::now()) {
	std::time_t const stamp_time = stdch::system_clock::to_time_t(stamp);
	std::tm const stamp_tm = *std::localtime(&stamp_time);
	char ret[1024];
	std::strftime(ret, 1024, fmt.data(), &stamp_tm);
	return ret;
}
} // namespace

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
		print(Level::warn, ktl::str_format("Already logging to file [{}]", g_file->path), false);
		return std::nullopt;
	}
	if (auto file = std::ofstream(path); !file) {
		error("Failed to create log file [{}]", path);
		return std::nullopt;
	}
	g_file.emplace(std::move(path));
	print(Level::debug, ktl::str_format("Logging to file [{}]", g_file->path), false);
	print(Level::info, ktl::str_format("jukebox v{} | {}", jukebox_version, timeStr("%a %F (%Z)")));
	return File(true);
}

void Log::print(Level level, std::string_view message, bool file) {
	static constexpr char levels[] = {'E', 'W', 'I', 'D'};
	std::ostream& out = level == Level::error ? std::cerr : std::cout;
	auto line = ktl::str_format("[{}] {} [{}]\n", levels[std::size_t(level)], message, timeStr());
	out << line;
	if (file && g_file) { g_file->queue.push(std::move(line)); }
}
} // namespace jk
