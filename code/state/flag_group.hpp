#ifndef FLAG_GROUP_HPP
#define FLAG_GROUP_HPP

#include <bitset>

#include <magic_enum.hpp>

using std::bitset;

template <typename T>
class flag_group {
public:
	flag_group() {};

	void raise(T value) {
		data.set(static_cast<size_t>(value));
	}

	void drop(T value) {
		data.reset(static_cast<size_t>(value));
	}

	void clear() {
		data.reset();
	}

	bool has(T value) {
		return data.test(static_cast<size_t>(value));
	}

	template <typename... Args>
	bool has(T value, Args... args) {
		return data.test(static_cast<size_t>(value)) && has(args...);
	}

private:
	bitset<magic_enum::enum_count<T>()> data;
};

#endif