#pragma once
#include <string>
#include <vector>

namespace jk {
struct Playlist {
	static constexpr std::string_view default_prefix_v = "jukebox playlist";
	std::vector<std::string> tracks;

	static bool valid(char const* path, bool silent, std::string_view prefix = default_prefix_v);
	std::size_t load(char const* path, std::string_view prefix = default_prefix_v);
	bool save(char const* path, std::string_view prefix = default_prefix_v);
};
} // namespace jk
