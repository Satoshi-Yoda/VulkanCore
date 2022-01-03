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

#include "Technician.h"
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

class Technician;

class Wing {
public:
	Wing(size_t count = 0);
	~Wing();

	shared_ptr<Task> task(const function<void()> func, const set<shared_ptr<Task>> dependencies = set<shared_ptr<Task>>());
	shared_ptr<Task> idleTask(const function<void()> func);
	void stopIdleTask(const shared_ptr<Task> task);
	void join();
	void finish();
	Technician* findCurrentTechnician();
	double initTime();
	double workTime();
	pair<size_t, size_t> specialistsIdRange();

	uint32_t cpuThreads;

	mutex mtx;
	condition_variable task_cv;
	condition_variable join_cv;
	condition_variable finish_cv;

	queue<shared_ptr<Task>> idleTasks;
	set<shared_ptr<Task>> stoppingIdleTasks;
	queue<shared_ptr<Task>> availableTasks;
	set<shared_ptr<Task>> blockedTasks;
	bool quitFlag = false;

private:
	list<Technician> specialists;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> ready;

};
