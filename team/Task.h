#pragma once

#include <functional>
#include <memory>
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using std::function;
using std::set;
using std::shared_ptr;

enum Speciality {
	ST_CPU,
	ST_GPU,
	ST_PCI,
	ST_HDD,
};

const size_t SpecialityCount = 4;

struct Task {
	Task(Speciality speciality, function<void()> func) : speciality(speciality), func(func), cbFunc(nullptr) {};
	Task(Speciality speciality, function<void(VkCommandBuffer)> func) : speciality(speciality), func(nullptr), cbFunc(func) {};
	~Task() {};

	Speciality speciality;
	function<void()> func;
	function<void(VkCommandBuffer)> cbFunc;
	set<shared_ptr<Task>> dependencies;
	set<shared_ptr<Task>> dependants;
	bool isIdle = false;
	bool done = false;
};
