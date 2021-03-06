#include <jk_version.hpp>
#include <ktl/kformat.hpp>
#include <misc/version.hpp>
#include <cstdlib>
#include <cstring>

namespace jk {
namespace {
char const* advance(int& out, char const* in) noexcept {
	if (in && *in) {
		out = std::atoi(in);
		if (auto const ret = std::strchr(in, '.')) { return ret + 1; }
	}
	return nullptr;
}
} // namespace

Version Version::parse(std::string_view str) noexcept {
	Version ret;
	if (!str.empty() && str[0] == 'v') { str = str.substr(1); }
	char const* ch = advance(ret.major, str.data());
	ch = advance(ret.minor, ch);
	ch = advance(ret.patch, ch);
	ch = advance(ret.tweak, ch);
	return ret;
}

Version Version::app() noexcept {
	static Version const ret = parse(jukebox_version);
	return ret;
}

std::string Version::toString(bool full) const noexcept {
	if (full) {
		return ktl::kformat("v{}.{}.{}.{}", major, minor, patch, tweak);
	} else {
		return ktl::kformat("v{}.{}", major, minor);
	}
}
} // namespace jk
