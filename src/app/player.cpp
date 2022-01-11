#include <app/player.hpp>
#include <app/playlist.hpp>
#include <misc/log.hpp>

namespace jk {
namespace {
template <typename Container>
void extractPaths(Container const& paths, std::vector<std::string>& out, capo::Music& music, std::size_t& out_total) {
	out.reserve(out.size() + paths.size());
	for (std::string_view path : paths) {
		if (path.empty()) { continue; }
		auto const extIdx = path.find_last_of('.');
		if (extIdx == std::string_view::npos) { continue; }
		auto const ext = path.substr(extIdx);
		if (ext == ".txt") {
			Playlist list;
			if (auto loaded = list.load(path.data()); loaded > 0) {
				Log::debug("[Player] loaded {} tracks from playlist [{}]", loaded, path);
				extractPaths(list.tracks, out, music, out_total);
			}
		} else if (music.open(path)) {
			out.push_back(std::string(path));
			Log::info("[Player] Added [{}]", path);
			++out_total;
		} else {
			Log::info("[Player] Skipped [{}]", path);
		}
	}
}

template <typename T, typename U>
constexpr std::size_t getIndex(T const& elems, U const& u) noexcept {
	for (std::size_t i = 0; i < elems.size(); ++i) {
		if (elems[i] == u) { return i; }
	}
	return std::string_view::npos;
}
} // namespace

Player::Player(ktl::not_null<capo::Instance*> capo) : m_music(capo), m_capo(capo) {}

bool Player::add(std::span<const std::string> paths) {
	std::size_t added{};
	capo::Music music(m_capo);
	extractPaths(paths, m_paths, music, added);
	return added > 0;
}

bool Player::push(std::string path, bool autoplay) {
	std::size_t added{};
	capo::Music music(m_capo);
	std::string const paths[] = {std::move(path)};
	extractPaths(std::span(paths), m_paths, music, added);
	if (added > 0 && autoplay) { navLast(); }
	return added > 0;
}

bool Player::pop() noexcept { return pop(path()); }

bool Player::pop(std::string_view path) noexcept {
	if (m_paths.empty()) { return false; }
	bool const replay = playing();
	auto const index = getIndex(m_paths, path);
	bool const backstep = index <= m_head;
	if (index != std::string_view::npos) { Log::info("[Player] Removed [{}]", path); }
	if (path == this->path()) {
		stop();
		std::erase(m_paths, path);
		if (backstep) { m_head = m_head > 0 ? m_head - 1 : 0; }
		open(replay);
		return true;
	}
	auto const ret = std::erase(m_paths, path) > 0;
	if (ret && backstep) { m_head = m_head > 0 ? m_head - 1 : 0; }
	return ret;
}

bool Player::open(bool autoplay) {
	if (empty()) { return false; }
	stop();
	if (open()) {
		if (autoplay) { play(); }
		return true;
	}
	return false;
}

void Player::clear() {
	stop();
	m_paths.clear();
	m_head = 0;
	Log::info("[Player] Playlist cleared");
}

Player& Player::play() {
	if (empty()) { return *this; }
	if (m_status != Status::ePaused) {
		if (m_mode == Mode::ePreload) {
			if (auto pcm = capo::PCM::fromFile(path().data()); !pcm) {
				return preloadFail(true);
			} else {
				if (!m_music.preload(std::move(*pcm))) { return preloadFail(true); }
			}
		} else {
			if (!open()) { return *this; }
		}
	}
	if (m_status != Status::ePlaying && m_music.play()) { transition(Status::ePlaying); }
	return *this;
}

Player& Player::stop() {
	m_music.stop();
	transition(Status::eStopped);
	return *this;
}

Player& Player::pause() {
	if (playing() && m_music.pause()) { transition(Status::ePaused); }
	return *this;
}

Player& Player::seek(capo::Time stamp) {
	m_music.seek(stamp);
	return *this;
}

Player& Player::gain(float gain) {
	if (gain >= 0.0f) {
		m_music.gain(gain);
		m_cachedGain = -1.0f;
	}
	return *this;
}

float Player::gain() const { return muted() ? m_cachedGain : m_music.gain(); }

Player& Player::mute() {
	if (!muted()) {
		m_cachedGain = m_music.gain();
		m_music.gain(0.0f);
		Log::info("[Player] Muted");
	}
	return *this;
}

Player& Player::unmute() {
	if (muted()) {
		m_music.gain(m_cachedGain);
		m_cachedGain = -1.0f;
		Log::info("[Player] Unmuted");
	}
	return *this;
}

void Player::update() {
	if (playing() && m_music.state() == capo::State::eStopped) {
		if (isLastTrack()) {
			transition(Status::eStopped);
		} else {
			Log::info("[Player] Autoplaying next track [{}]", m_paths[m_head + 1]);
			navNext();
		}
	}
}

Player& Player::navFirst() { return navIndex(0); }
Player& Player::navLast() { return navIndex(m_head = m_paths.empty() ? 0 : m_paths.size() - 1); }
Player& Player::navNext() { return navIndex(m_head + 1); }
Player& Player::navPrev() { return navIndex(m_head > 0 ? m_head - 1 : m_head); }

Player& Player::navIndex(std::size_t index) {
	if (index < m_paths.size()) {
		m_head = index;
		open(playing());
	}
	return *this;
}

Player& Player::swapTracks(std::size_t lhs, std::size_t rhs) noexcept {
	if (lhs >= m_paths.size() || rhs > m_paths.size()) { return *this; }
	if (m_head == lhs) {
		m_head = rhs;
	} else if (m_head == rhs) {
		m_head = lhs;
	}
	Log::info("[Player] Swapped track {} [{}] with track {} [{}]", lhs, m_paths[lhs], rhs, m_paths[rhs]);
	std::swap(m_paths[lhs], m_paths[rhs]);
	return *this;
}

Player& Player::mode(Mode mode) {
	if (m_mode != mode) {
		m_mode = mode;
		if (empty()) { return *this; }
		auto const pos = m_music.position();
		auto const replay = playing();
		stop();
		if (replay) {
			play();
			seek(pos);
		}
	}
	return *this;
}

void Player::transition(Status next) noexcept {
	switch (m_status) {
	case Status::eIdle: assert(next != Status::ePaused); break;
	case Status::ePlaying: assert(anyOf(next, Status::ePaused, Status::eStopped)); break;
	case Status::ePaused: assert(anyOf(next, Status::ePlaying, Status::eStopped)); break;
	case Status::eStopped: break;
	}
	m_status = next;
}

Player& Player::preloadFail(bool autoplay) {
	Log::error("[Player] Failed to preload [{}]!", path());
	m_mode = Mode::eStream;
	if (open() && autoplay) { m_music.play(); }
	return *this;
}

bool Player::open() {
	if (m_music.open(path())) { return true; }
	Log::error("[Player] Failed to open [{}]!", path());
	return false;
}
} // namespace jk
