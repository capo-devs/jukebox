#pragma once
#include <jk_common.hpp>
#include <ktl/str_format.hpp>
#include <optional>

namespace jk {

class Log {
  public:
	enum class Level { error, warn, info, debug };
	struct File;

	static Level minLevel() noexcept { return s_minLevel; }
	static void minLevel(Level level) noexcept { s_minLevel = level; }
	static std::optional<File> toFile(std::string path);

	template <typename... T>
	static void log(Level level, std::string_view fmt, T const&... t) {
		if (!skip(level) && level <= s_minLevel) { print(level, ktl::str_format(fmt, t...)); }
	}

	template <typename... T>
	static void error(std::string_view fmt, T const&... t) {
		log(Level::error, fmt, t...);
	}

	template <typename... T>
	static void warn(std::string_view fmt, T const&... t) {
		log(Level::warn, fmt, t...);
	}

	template <typename... T>
	static void info(std::string_view fmt, T const&... t) {
		log(Level::info, fmt, t...);
	}

	template <typename... T>
	static void debug(std::string_view fmt, T const&... t) {
		if constexpr (jk_debug) { log(Level::debug, fmt, t...); }
	}

  private:
	static constexpr bool skip(Level level) noexcept { return level == Level::debug && !jk_debug; }
	static void print(Level level, std::string_view msg, bool file = true);

	inline static Level s_minLevel = jk_debug ? Level::debug : Level::warn;
};

struct Log::File {
	File(File&&) noexcept;
	File& operator=(File) noexcept;
	~File() noexcept;

  private:
	File(bool active) noexcept : active(active) {}
	bool active{};
	friend class Log;
};
} // namespace jk
