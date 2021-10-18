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
			Log::info("[Player] added {}", path);
			++ret;
		}
	}
	return ret;
}
} // namespace

Player::Player(ktl::not_null<capo::Instance*> capo) : m_music(capo), m_capo(capo) {}

bool Player::push(std::string path, bool focus, bool autoplay) {
	capo::Music music(m_capo);
	if (music.open(path)) {
		m_paths.push_back(std::move(path));
		if (focus) { navLast(autoplay); }
		return true;
	}
	return false;
}

bool Player::pop() noexcept { return pop(path()); }

bool Player::pop(std::string_view path) noexcept {
	if (m_paths.empty()) { return false; }
	bool const playing = m_status == Status::ePlaying;
	bool const last = isLastTrack();
	if (path == this->path()) {
		stop();
		std::erase(m_paths, path);
		if (last) { m_head = m_paths.empty() ? 0 : m_paths.size() - 1; }
		open(playing);
		return true;
	}
	auto const ret = std::erase(m_paths, path) > 0;
	if (ret && last) { m_head = m_paths.empty() ? 0 : m_paths.size() - 1; }
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

Player& Player::play() {
	if (empty()) { return *this; }
	if (m_status != Status::ePaused && !m_music.open(path())) { return *this; }
	if (m_status != Status::ePlaying && m_music.play()) { m_status = Status::ePlaying; }
	return *this;
}

Player& Player::stop() {
	m_music.stop();
	m_status = Status::eStopped;
	return *this;
}

Player& Player::pause() {
	if (m_music.pause()) { m_status = Status::ePaused; }
	return *this;
}

Player& Player::seek(capo::Time stamp) {
	m_music.seek(stamp);
	return *this;
}

void Player::update() {
	if (m_status == Status::ePlaying && m_music.state() == capo::State::eStopped) {
		if (isLastTrack()) {
			m_status = Status::eStopped;
		} else {
			navNext(true);
		}
	}
}

Player& Player::navFirst(bool autoplay) { return navIndex(0, autoplay); }
Player& Player::navLast(bool autoplay) { return navIndex(m_head = m_paths.empty() ? 0 : m_paths.size() - 1, autoplay); }
Player& Player::navNext(bool autoplay) { return navIndex(m_head + 1, autoplay); }
Player& Player::navPrev(bool autoplay) { return navIndex(m_head > 0 ? m_head - 1 : m_head, autoplay); }

Player& Player::navIndex(std::size_t index, bool autoplay) {
	if (index < m_paths.size()) {
		m_head = index;
		open(autoplay);
	}
	return *this;
}

bool Player::add(std::span<const str_t> paths, bool focus, bool autoplay) {
	if (pushValidPaths(paths, m_paths, *m_capo) > 0) {
		if (focus) { navLast(autoplay); }
		return true;
	}
	return false;
}
} // namespace jk
