#pragma once
#include <capo/capo.hpp>
#include <memory>
#include <vector>

namespace jk {
using str_t = char const*;

class Player {
  public:
	enum class Status { eIdle, ePlaying, ePaused, eStopped };

	Player(Player&&) = delete;
	Player& operator=(Player&&) = delete;

	static std::unique_ptr<Player> make();

	bool add(std::span<str_t const> paths);
	bool push(std::string path, bool focus);
	bool pop(std::string_view path) noexcept;
	bool pop() noexcept;

	Player& play();
	Player& pause();
	Player& stop();
	Player& seek(capo::Time stamp);
	void update();

	Player& navFirst() { return (m_head = 0, play()); }
	Player& navLast() { return (m_head = m_paths.empty() ? 0 : m_paths.size() - 1, play()); }
	Player& navIndex(std::size_t index);
	Player& navNext() { return navIndex(m_head + 1); }
	Player& navPrev() { return m_head > 0 ? navIndex(m_head - 1) : *this; }

	capo::Music const& music() const noexcept { return m_music; }
	std::span<std::string const> paths() const noexcept { return m_paths; }
	std::size_t index() const noexcept { return m_head; }
	std::string_view path() const noexcept { return m_head < m_paths.size() ? m_paths[m_head] : std::string_view(); }
	bool empty() const noexcept { return m_paths.empty(); }
	std::size_t size() const noexcept { return m_paths.size(); }
	bool isLastTrack() const noexcept { return m_head + 1 == m_paths.size(); }

  private:
	Player() = default;

	capo::Instance m_instance;
	capo::Music m_music;
	std::vector<std::string> m_paths;
	std::size_t m_head{};
	Status m_status{};
};
} // namespace jk
