#pragma once
#include <jk_common.hpp>
#include <ktl/str_format.hpp>
#include <memory>

namespace jk {

class Log {
  public:
	enum class Level { error, warn, info, debug };

	struct File {
		struct Impl;
		std::unique_ptr<Impl> impl;

		File(std::string_view path) noexcept;
		File(File&&) noexcept;
		File& operator=(File&&) noexcept;
		~File();

		bool logging() const noexcept;
		void stop();
	};

	static Level minLevel() noexcept { return s_minLevel; }
	static void minLevel(Level level) noexcept { s_minLevel = level; }

	template <typename... T>
	static void log(Level level, std::string_view fmt, T const&... t) {
		if (!skip(level) && level <= s_minLevel) { print(level, ktl::format(fmt, t...)); }
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
	static void print(Level level, std::string_view msg);

	inline static Level s_minLevel = jk_debug ? Level::debug : Level::warn;
};
} // namespace jk
