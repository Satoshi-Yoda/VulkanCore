class Vector1 {
public:
	double start_x;
	double start_y;
	double finish_x;
	double finish_y;

	void print();
	void zero();
};

class Point {
public:
	double x;
	double y;
};

class Vector2 {
public:
	Point start;
	Point finish;
};
