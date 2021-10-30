#pragma once
#include <ktl/stack_string.hpp>
#include <sstream>
#include <string>
#include <unordered_map>

namespace jk {
class Props {
	using Storage = std::unordered_map<std::string, std::string>;

  public:
	using const_iterator = Storage::const_iterator;

	enum class Result { eNone, eKey, eKeyValue };
	template <typename T>
	static constexpr bool stringy_v = std::is_convertible_v<T, std::string>;

	bool add(bool overwrite, std::string key) { return add<std::string_view>(std::move(key), {}, overwrite, true); }
	template <typename T>
	bool add(bool overwrite, std::string key, T const& value) {
		return add(std::move(key), value, overwrite, true);
	}

	std::size_t load(char const* path, bool overwrite = true);
	bool save(char const* path) const;

	Result search(std::string const& key) const noexcept;
	bool contains(std::string const& key) const noexcept { return search(key) != Result::eNone; }
	bool hasValue(std::string const& key) const noexcept { return search(key) == Result::eKeyValue; }
	template <typename T>
	T get(std::string const& key, T const& fallback = T{}) const noexcept;
	template <typename T>
	bool set(std::string const& key, T const& value) noexcept {
		return add(key, value, true, false);
	}

	std::size_t size() const noexcept { return m_storage.size(); }
	bool empty() const noexcept { return m_storage.empty(); }
	const_iterator begin() const noexcept { return m_storage.begin(); }
	const_iterator cbegin() const noexcept { return m_storage.begin(); }
	const_iterator end() const noexcept { return m_storage.end(); }
	const_iterator cend() const noexcept { return m_storage.end(); }

  private:
	template <typename T>
	void setValue(std::string& out, T const& value);

	template <typename T>
	bool add(std::string key, T const& value, bool overwrite, bool make);

	Storage m_storage;
};

// impl

template <typename T>
T Props::get(std::string const& key, T const& fallback) const noexcept {
	if (auto it = m_storage.find(key); it != m_storage.end()) {
		if constexpr (stringy_v<T>) {
			return T(it->second);
		} else {
			T ret;
			std::stringstream str(it->second);
			str >> ret;
			return ret;
		}
	}
	return fallback;
}

template <typename T>
void Props::setValue(std::string& out, T const& value) {
	if constexpr (stringy_v<T>) {
		out = value;
	} else {
		std::stringstream str;
		str << value;
		out = str.str();
	}
}

template <typename T>
bool Props::add(std::string key, T const& value, bool overwrite, bool make) {
	if (auto it = m_storage.find(key); it != m_storage.end()) {
		if (overwrite) {
			setValue(it->second, value);
			return true;
		}
		return false;
	}
	if (make) {
		auto [it, _] = m_storage.insert_or_assign(std::move(key), std::string());
		setValue(it->second, value);
		return true;
	}
	return false;
}
} // namespace jk
