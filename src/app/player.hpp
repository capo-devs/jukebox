#pragma once
#include <capo/capo.hpp>
#include <ktl/not_null.hpp>
#include <vector>

namespace jk {
using str_t = char const*;

class Player {
  public:
	enum class Status { eIdle, ePlaying, ePaused, eStopped };

	Player(ktl::not_null<capo::Instance*> capo);

	bool add(std::span<str_t const> paths, bool focus, bool autoplay);
	bool push(std::string path, bool focus, bool autoplay);
	bool pop(std::string_view path) noexcept;
	bool pop() noexcept;
	bool open(bool autoplay);

	Player& play();
	Player& pause();
	Player& stop();
	Player& seek(capo::Time stamp);
	Player& gain(float gain) { return (m_music.gain(gain), *this); }
	void update();

	Player& navFirst(bool autoplay);
	Player& navLast(bool autoplay);
	Player& navNext(bool autoplay);
	Player& navPrev(bool autoplay);
	Player& navIndex(std::size_t index, bool autoplay);

	capo::Music const& music() const noexcept { return m_music; }
	std::span<std::string const> paths() const noexcept { return m_paths; }
	std::size_t index() const noexcept { return m_head; }
	std::string_view path() const noexcept { return m_head < m_paths.size() ? m_paths[m_head] : std::string_view(); }
	bool empty() const noexcept { return m_paths.empty(); }
	std::size_t size() const noexcept { return m_paths.size(); }
	bool isLastTrack() const noexcept { return m_head + 1 == m_paths.size(); }
	Status status() const noexcept { return m_status; }

  private:
	capo::Music m_music;
	std::vector<std::string> m_paths;
	ktl::not_null<capo::Instance*> m_capo;
	std::size_t m_head{};
	Status m_status{};
};
} // namespace jk
