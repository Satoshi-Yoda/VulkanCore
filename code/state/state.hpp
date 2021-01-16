template <typename T>
class state {
public:
	state() {};
	state(T value) : value(value) {};

	T& operator= (T& value) {
		return this->value = value;
	}

	bool is(T value) {
		return value == this->value;
	}

	template <typename... Args>
	bool is(T value, Args... args) {
		return (value == this->value) || is(args...);
	}

private:
	T value;
};
