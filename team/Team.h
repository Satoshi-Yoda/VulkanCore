#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Specialist.h"
#include "Task.h"

#include "../core/Rocks.h"

using std::array;
using std::condition_variable;
using std::list;
using std::mutex;
using std::queue;
using std::set;
using std::shared_ptr;

class Specialist;

class Team {
public:
	Team();
	~Team();

	void initGpuSpecialists(Rocks& rocks);
	shared_ptr<Task> task(const Speciality speciality, const function<void()> func, const set<shared_ptr<Task>> dependencies = set<shared_ptr<Task>>()); // TODO make cpu_task(...), etc
	shared_ptr<Task> gpuTask(const function<void(VkCommandBuffer)> func, const set<shared_ptr<Task>> dependencies = set<shared_ptr<Task>>());
	void join();
	// bool wait(std::chrono::milliseconds time);
	Specialist* findCurrentSpecialist();
	double initTime();
	double workTime();

	mutex mutex;
	array<condition_variable, SpecialityCount> cvs;
	condition_variable ready_cv;

	array<queue<shared_ptr<Task>>, SpecialityCount> availableTasks;
	set<shared_ptr<Task>> blockedTasks;
	bool quitFlag = false;

private:
	list<Specialist> specialists;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> ready;

};
