#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <utility>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Specialist.h"
#include "Task.h"

#include "../core/Rocks.h"

using std::array;
using std::condition_variable;
using std::list;
using std::mutex;
using std::pair;
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
	shared_ptr<Task> idleTask(const Speciality speciality, const function<void()> func);
	void stopIdleTask(const shared_ptr<Task> task);
	void join();
	void finish();
	// TODO implement joinTasks(set<shared_ptr<Task>> tasks);
	// bool wait(std::chrono::milliseconds time);
	Specialist* findCurrentSpecialist();
	double initTime();
	double workTime();
	pair<size_t, size_t> specialistsIdRange(Speciality speciality);

	uint32_t cpuThreads;

	mutex mtx;
	array<condition_variable, SpecialityCount> cvs;
	condition_variable join_cv;
	condition_variable finish_cv;

	array<queue<shared_ptr<Task>>, SpecialityCount> idleTasks;
	array<set<shared_ptr<Task>>, SpecialityCount> stoppingIdleTasks;
	array<queue<shared_ptr<Task>>, SpecialityCount> availableTasks;
	set<shared_ptr<Task>> blockedTasks;
	bool quitFlag = false;

private:
	list<Specialist> specialists;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> ready;

};
