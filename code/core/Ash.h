#pragma once

#include <mutex>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using std::mutex;
using std::string;
using std::vector;

class Ash {
public:
	Ash();
	~Ash();

	Ash& operator()(string message);

	string current;
	mutex mtx;
};

void operator>>(bool status, Ash& ash);
void operator>>(VkResult status, Ash& ash);

//TODO maybe make ash() function
