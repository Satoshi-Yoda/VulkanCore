#ifndef STATE_HPP
#define STATE_HPP

template <typename T>
class state {
public:
	state() {};
	state(T value) : value(value) {};

	inline T& operator= (T& value) noexcept {
		return this->value = value;
	}

	inline bool is(T value) noexcept {
		return value == this->value;
	}
	template <typename... Args>
	inline bool is(T value, Args... args) noexcept {
		return is(value) || is(args...);
	}

private:
	T value;
};

#endif