#pragma once
#include <ktl/move_only_function.hpp>
#include <ktl/tmutex.hpp>
#include <misc/dummy_lock.hpp>
#include <misc/handle.hpp>
#include <cstdint>
#include <iterator>
#include <vector>

namespace jk {
enum class DelegatePolicy { eSingleThreaded, eMultiThreaded };

template <DelegatePolicy Policy, typename... Args>
	requires(!std::is_void_v<Args> && ...)
class TDelegate;

template <typename... Args>
using Delegate = TDelegate<DelegatePolicy::eSingleThreaded, Args...>;

template <typename... Args>
using MTDelegate = TDelegate<DelegatePolicy::eMultiThreaded, Args...>;

///
/// \brief Models an ordered list of unsubscribe-able callbacks
///
template <DelegatePolicy Policy, typename... Args>
	requires(!std::is_void_v<Args> && ...)
class TDelegate {
  public:
	///
	/// \brief Alias for callback type
	///
	using Callback = ktl::move_only_function<void(Args const&...)>;
	///
	/// \brief Type for identifying each registered callback
	///
	using ID = THandle<>;
	///
	/// \brief RAII wrapper for subscribing callbacks: unsubs all on destruction
	///
	class Signal;

	struct const_iterator;
	using iterator = const_iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;

	[[nodiscard]] Signal signal() noexcept { return this; }

	[[nodiscard]] ID push(Callback&& cb) {
		if (cb) { Lock<Storage>(m_entries)->push_back({std::move(cb), ++m_next.value}); }
		return m_next;
	}

	void pop(ID handle) {
		Lock<Storage> lock(m_entries);
		std::erase_if(*lock, [handle](Entry const& e) { return e.handle == handle; });
	}

	void operator()(Args const&... args) const {
		for (Callback const& callback : *this) { callback(args...); }
	}

	bool operator()(ID single, Args const&... args) const {
		Lock<Storage const> lock(m_entries);
		auto it = std::find_if(lock->begin(), lock->end(), [single](Entry const& e) { return e.handle == single; });
		if (it != lock->end()) {
			it->callback(args...);
			return true;
		}
		return false;
	}

	std::size_t size() const noexcept { return Lock<Storage const>(m_entries)->size(); }
	void clear() noexcept { Lock<Storage>(m_entries)->clear(); }

	iterator begin() const noexcept { return Lock<Storage const>(m_entries)->begin(); }
	iterator end() const noexcept { return Lock<Storage const>(m_entries)->end(); }
	const_iterator cbegin() const noexcept { return Lock<Storage const>(m_entries)->begin(); }
	const_iterator cend() const noexcept { return Lock<Storage const>(m_entries)->end(); }
	reverse_iterator rbegin() const noexcept { return reverse_iterator(Lock<Storage const>(m_entries)->end()); }
	reverse_iterator rend() const noexcept { return reverse_iterator(Lock<Storage const>(m_entries)->begin()); }

  private:
	struct Entry {
		Callback callback;
		ID handle;
	};

	static constexpr bool is_mt = Policy == DelegatePolicy::eMultiThreaded;
	template <typename T>
	using Mutex = std::conditional_t<is_mt, ktl::strict_tmutex<T, std::mutex>, DummyMutex<T>>;
	template <typename T>
	using Lock = typename std::conditional_t<is_mt, ktl::tlock<T, std::scoped_lock, std::mutex>, DummyLock<T>>;

	using Storage = std::vector<Entry>;
	using Iter = typename Storage::const_iterator;

	Mutex<Storage> m_entries;
	ID m_next{};

	friend struct const_iterator;
};

template <DelegatePolicy Policy, typename... Args>
	requires(!std::is_void_v<Args> && ...)
class TDelegate<Policy, Args...>::Signal {
  public:
	Signal() = default;
	Signal(Signal&& rhs) noexcept : Signal() { exchg(*this, rhs); }
	Signal& operator=(Signal rhs) noexcept { return (exchg(*this, rhs), *this); }
	~Signal() noexcept { reset(); }

	void operator+=(Callback&& callback) { push(std::move(callback)); }

	bool push(Callback&& callback) {
		if (m_delegate) {
			auto handle = m_delegate->push(std::move(callback));
			if (handle != null_handle_v) {
				m_handles.push_back(std::move(handle));
				return true;
			}
		}
		return false;
	}

	void reset() noexcept {
		if (m_delegate) {
			for (auto const& handle : m_handles) { m_delegate->pop(handle); }
		}
		m_handles.clear();
	}

  private:
	Signal(TDelegate* delegate) noexcept : m_delegate(delegate) {}

	static void exchg(Signal& lhs, Signal& rhs) noexcept {
		std::swap(lhs.m_handles, rhs.m_handles);
		std::swap(lhs.m_delegate, rhs.m_delegate);
	}

	std::vector<ID> m_handles;
	TDelegate* m_delegate{};

	friend class TDelegate;
};

template <DelegatePolicy Policy, typename... Args>
	requires(!std::is_void_v<Args> && ...)
struct TDelegate<Policy, Args...>::const_iterator {
	using iterator_category = std::bidirectional_iterator_tag;
	using type = typename TDelegate::Callback;
	using pointer = type const*;
	using reference = type const&;

	type const& operator*() const noexcept { return iter->callback; }
	const_iterator& operator++() noexcept { return (++iter, *this); }
	const_iterator& operator--() noexcept { return (--iter, *this); }
	const_iterator operator++(int) noexcept {
		auto ret = *this;
		++(*this);
		return ret;
	}
	const_iterator operator--(int) noexcept {
		auto ret = *this;
		--(*this);
		return ret;
	}

	bool operator==(const_iterator const&) const = default;

  private:
	const_iterator(typename TDelegate::Iter iter) noexcept : iter(iter) {}
	typename TDelegate::Iter iter;
	friend class TDelegate;
};
} // namespace jk
