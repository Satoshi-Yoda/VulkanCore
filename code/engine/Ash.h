#ifndef ASH_H
#define ASH_H

#include <vector>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using std::vector;
using std::string;

class Ash {
public:
	Ash();
	~Ash();

	Ash& operator()(string message);

	string current;
	vector<string> messages;
};

void operator>>(bool status, Ash& ash);
void operator>>(VkResult status, Ash& ash);

#endif