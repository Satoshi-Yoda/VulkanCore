#pragma once

#include <bitset>

#include <magic_enum.hpp>

using std::bitset;

template <typename T>
class flag_group {
public:
	flag_group() {};

	inline void raise(T value) noexcept {
		data.set(static_cast<size_t>(value));
	}
	template <typename... Args>
	inline void raise(T value, Args... args) noexcept {
		raise(value);
		raise(args...);
	}

	inline void drop(T value) noexcept {
		data.reset(static_cast<size_t>(value));
	}
	template <typename... Args>
	inline void drop(T value, Args... args) noexcept {
		drop(value);
		drop(args...);
	}

	inline bool has(T value) noexcept {
		return data.test(static_cast<size_t>(value));
	}
	template <typename... Args>
	inline bool has(T value, Args... args) noexcept {
		return has(value) && has(args...);
	}

	inline void clear() noexcept {
		data.reset();
	}

private:
	bitset<magic_enum::enum_count<T>()> data;
};
