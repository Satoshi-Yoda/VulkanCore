#include "Ash.h"

#include <vector>
#include <string>
#include <iostream>

using namespace std;

void operator>>(bool status, Ash& ash) {
	if (status == false) {
		cout << "ASH " << ash.current << endl;
		ash.messages.push_back(ash.current);
	}
	ash.current = "";
}

void operator>>(VkResult status, Ash& ash) {
	if (status != VK_SUCCESS) {
		cout << "ASH " << status << " " << ash.current << endl;
		ash.messages.push_back(ash.current);
	}
	ash.current = "";
}

Ash::Ash() {}

Ash::~Ash() {}

Ash& Ash::operator()(string message) {
	this->current = message;
	return *this;
}
