#include "Ash.h"

#include <vector>
#include <string>
#include <iostream>

using namespace std;

Ash::Ash() {}

Ash::~Ash() {}

Ash& Ash::operator()(string message) {
	this->mtx.lock();
		this->current = message;
	this->mtx.unlock();
	return *this;
}

void operator>>(bool status, Ash& ash) {
	ash.mtx.lock();
		if (status == false) {
			cout << "ASH " << ash.current << endl;
		}
		ash.current = "";
	ash.mtx.unlock();
}

void operator>>(VkResult status, Ash& ash) {
	ash.mtx.lock();
		if (status != VK_SUCCESS) {
			cout << "ASH " << status << " " << ash.current << endl;
		}
		ash.current = "";
	ash.mtx.unlock();
}
