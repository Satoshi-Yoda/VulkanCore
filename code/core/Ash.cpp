#include "Ash.h"

#include <vector>
#include <string>
#include <iostream>

using namespace std;

void operator>>(bool status, Ash& ash) {
	ash.mtx.lock();
		if (status == false) {
			cout << "ASH " << ash.current << endl;
			ash.messages.push_back(ash.current); // TODO what for?
		}
		ash.current = "";
	ash.mtx.unlock();
}

void operator>>(VkResult status, Ash& ash) {
	ash.mtx.lock();
		if (status != VK_SUCCESS) {
			cout << "ASH " << status << " " << ash.current << endl;
			ash.messages.push_back(ash.current); // TODO what for?
		}
		ash.current = "";
	ash.mtx.unlock();
}

Ash::Ash() {}

Ash::~Ash() {}

Ash& Ash::operator()(string message) {
	this->mtx.lock();
		this->current = message;
	this->mtx.unlock();
	return *this;
}
