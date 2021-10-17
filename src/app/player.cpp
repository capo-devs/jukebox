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
			++ret;
		}
	}
	return ret;
}
} // namespace

bool Player::push(std::string path, bool focus) {
	capo::Music music(&m_instance);
	if (music.open(path)) {
		m_paths.push_back(std::move(path));
		if (focus) { navLast(); }
		return true;
	}
	return false;
}

bool Player::pop() noexcept { return pop(path()); }

bool Player::pop(std::string_view path) noexcept {
	if (m_paths.empty() || m_head >= m_paths.size()) { return false; }
	auto const ret = std::erase(m_paths, path) > 0;
	if (m_head >= m_paths.size()) { navLast(); }
	return ret;
}

Player& Player::play() {
	if (!path().empty()) {
		m_music.open(path());
		m_music.play();
		m_status = Status::ePlaying;
	}
	return *this;
}

Player& Player::stop() {
	m_music.stop();
	m_status = Status::eStopped;
	return *this;
}

Player& Player::pause() {
	m_music.pause();
	m_status = Status::ePaused;
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
			navNext();
		}
	}
}

Player& Player::navIndex(std::size_t index) {
	if (index < m_paths.size()) {
		m_head = index;
		return play();
	}
	return navLast();
}

std::unique_ptr<Player> Player::make() {
	auto ret = std::unique_ptr<Player>(new Player); // make_unique cannot call private constructor
	if (ret->m_instance.valid()) { return ret; }
	Log::error("Invalid capo instance");
	return {};
}

bool Player::add(std::span<const str_t> paths) { return pushValidPaths(paths, m_paths, m_instance) > 0; }
} // namespace jk
