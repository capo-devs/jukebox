#include <app/props.hpp>
#include <fstream>
#include <vector>

namespace jk {
namespace {
std::vector<std::string> readLines(char const* path) {
	std::vector<std::string> ret;
	if (auto file = std::ifstream(path)) {
		for (std::string line; std::getline(file, line); line.clear()) { ret.push_back(std::move(line)); }
	}
	return ret;
}

bool writeLines(char const* path, std::vector<std::string> const& lines) {
	if (lines.empty()) { return false; }
	if (auto file = std::ofstream(path)) {
		for (auto const& line : lines) { file << line << '\n'; }
		return true;
	}
	return false;
}

std::pair<std::string, std::string> extractKeyValue(std::string line) {
	if (auto const eq = line.find('='); eq != std::string::npos) { return {line.substr(0, eq), line.substr(eq + 1)}; }
	return {std::move(line), {}};
}

std::string flattenLine(std::string key, std::string value) {
	auto ret = std::move(key);
	if (!value.empty()) {
		ret += '=';
		ret += std::move(value);
	}
	return ret;
}
} // namespace

Props::Result Props::search(std::string const& key) const noexcept {
	if (auto it = m_storage.find(key); it != m_storage.end()) { return it->second.empty() ? Result::eKey : Result::eKeyValue; }
	return Result::eNone;
}

std::size_t Props::load(const char* path, bool overwrite) {
	std::size_t ret{};
	auto lines = readLines(path);
	for (std::string& line : lines) {
		if (!line.empty() && line[0] != '#') {
			auto kv = extractKeyValue(std::move(line));
			if (!kv.second.empty()) {
				add(overwrite, std::move(kv.first), kv.second);
			} else {
				add(overwrite, std::move(kv.first));
			}
			++ret;
		}
	}
	return ret;
}

bool Props::save(char const* path) const {
	std::unordered_map<std::string_view, std::string_view> view = {m_storage.begin(), m_storage.end()};
	auto lines = readLines(path);
	for (auto& line : lines) {
		if (line.empty() || line[0] == '#') { continue; }
		auto kv = extractKeyValue(std::move(line));
		if (auto it = view.find(kv.first); it != view.end()) {
			kv.second = it->second;
			view.erase(it);
		}
		line = flattenLine(std::move(kv.first), std::move(kv.second));
	}
	for (auto const& [key, value] : view) { lines.push_back(flattenLine(std::string(key), std::string(value))); }
	return writeLines(path, lines);
}
} // namespace jk
