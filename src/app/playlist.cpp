#include <app/playlist.hpp>
#include <misc/log.hpp>
#include <misc/version.hpp>
#include <fstream>
#include <optional>

namespace jk {
namespace {
std::optional<Version> getVersion(std::string_view header, std::string_view prefix) noexcept {
	if (header.empty() || header[0] != '#') { return std::nullopt; }
	header = header.substr(1);
	if (auto it = header.find(prefix); it != std::string_view::npos) {
		auto const version = Version::parse(header.substr(it + prefix.size()));
		return version;
	}
	return std::nullopt;
}
} // namespace

bool Playlist::valid(char const* path, bool silent, std::string_view prefix) {
	auto file = std::ifstream(path);
	if (!file) {
		if (!silent) { Log::warn("[Playlist] Failed to open [{}]", path); }
		return false;
	}
	std::string line;
	if (!std::getline(file, line)) {
		if (!silent) { Log::warn("[Playlist] Failed to read [{}]", path); }
		return false;
	}
	auto const ver = getVersion(line, prefix);
	if (!ver) {
		if (!silent) { Log::warn("[Playlist] Invalid playlist [{}]", path); }
		return false;
	}
	if (*ver == Version() || !Version::app().compatible(*ver)) {
		if (!silent) { Log::warn("[Playlist] Incompatible version [{}]", ver->toString(true).data()); }
		return false;
	}
	return true;
}

std::size_t Playlist::load(char const* path, std::string_view prefix) {
	if (!valid(path, false, prefix)) { return 0; }
	auto file = std::ifstream(path);
	std::size_t ret{};
	for (std::string line; std::getline(file, line); line.clear()) {
		if (!line.empty() && line[0] != '#') {
			tracks.push_back(std::move(line));
			++ret;
		}
	}
	return ret;
}

bool Playlist::save(char const* path, std::string_view prefix) {
	if (auto file = std::ofstream(path, std::ios::trunc)) {
		file << "# " << prefix << ' ' << Version::app().toString().data() << "\n\n";
		file << "#\n";
		file << "# Lines starting with # are ignored, except the first line (header)\n";
		file << "# Header must be in the above format (" << prefix << " <version>)\n";
		file << "# Tracks should be absolute paths\n";
		file << "#\n\n";
		for (auto const& track : tracks) { file << track << '\n'; }
		Log::info("[Playlist] Save to [{}] successful", path);
		return true;
	}
	return false;
}
} // namespace jk
