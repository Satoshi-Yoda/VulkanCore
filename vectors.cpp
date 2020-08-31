#include <iostream>

#include "vectors.h"

using namespace std;

void Vector1::print() {
	cout << "Class Start "  << start_x  << ", " << start_y  << endl;
	cout << "Class Finish " << finish_x << ", " << finish_y << endl;
}

void Vector1::zero() {
	start_x = 0;
	start_y = 0;
	finish_x = 0;
	finish_y = 0;
}
