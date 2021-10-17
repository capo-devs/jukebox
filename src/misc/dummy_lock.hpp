#pragma once

namespace jk {
template <typename T>
struct DummyMutex {
	using type = T;

	T t;
};

template <typename T>
struct DummyLock {
	using type = T;

	T& t;

	template <typename U>
	constexpr DummyLock(U&& out) noexcept : t(out.t) {}

	constexpr T& operator*() const noexcept { return t; }
	constexpr T* operator->() const noexcept { return &t; }
};

template <typename T>
DummyLock(DummyMutex<T>&) -> DummyLock<T>;
template <typename T>
DummyLock(DummyMutex<T> const&) -> DummyLock<T const>;
} // namespace jk
