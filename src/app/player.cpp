#include <app/player.hpp>
#include <misc/log.hpp>

namespace jk {
namespace {
std::size_t pushValidPaths(std::span<str_t const> paths, std::vector<std::string>& out, capo::Instance& instance) {
	capo::Music music(&instance);
	std::size_t ret{};
	out.reserve(out.size() + paths.size());
	for (std::string_view path : paths) {
		if (!path.empty() && music.open(path)) {
			out.push_back(std::string(path));
			Log::info("[Player] added [{}]", path);
			++ret;
		} else {
			Log::info("[Player] skipped [{}]", path);
		}
	}
	return ret;
}

template <typename T, typename U>
constexpr std::size_t getIndex(T const& elems, U const& u) noexcept {
	for (std::size_t i = 0; i < elems.size(); ++i) {
		if (elems[i] == u) { return i; }
	}
	return std::string::npos;
}
} // namespace

Player::Player(ktl::not_null<capo::Instance*> capo) : m_music(capo), m_capo(capo) {}

bool Player::add(std::span<const str_t> paths) {
	if (pushValidPaths(paths, m_paths, *m_capo) > 0) { return true; }
	return false;
}

bool Player::push(std::string path, bool autoplay) {
	capo::Music music(m_capo);
	if (music.open(path)) {
		m_paths.push_back(std::move(path));
		if (autoplay) { navLast(); }
		return true;
	}
	return false;
}

bool Player::pop() noexcept { return pop(path()); }

bool Player::pop(std::string_view path) noexcept {
	if (m_paths.empty()) { return false; }
	bool const replay = playing();
	bool const backstep = getIndex(m_paths, path) <= m_head;
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
	if (m_music.open(path())) {
		if (autoplay) { play(); }
		return true;
	}
	return false;
}

void Player::clear() {
	stop();
	m_paths.clear();
	m_head = 0;
}

Player& Player::play() {
	if (empty()) { return *this; }
	if (m_status != Status::ePaused && !m_music.open(path())) { return *this; }
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
	}
	return *this;
}

Player& Player::unmute() {
	if (muted()) {
		m_music.gain(m_cachedGain);
		m_cachedGain = -1.0f;
	}
	return *this;
}

void Player::update() {
	if (playing() && m_music.state() == capo::State::eStopped) {
		if (isLastTrack()) {
			transition(Status::eStopped);
		} else {
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
	std::swap(m_paths[lhs], m_paths[rhs]);
	return *this;
}

void Player::transition(Status next) noexcept {
	switch (m_status) {
	case Status::eIdle: assert(next == Status::ePlaying); break;
	case Status::ePlaying: assert(anyOf(next, Status::ePaused, Status::eStopped)); break;
	case Status::ePaused: assert(anyOf(next, Status::ePlaying)); break;
	case Status::eStopped: break;
	}
	m_status = next;
}
} // namespace jk
