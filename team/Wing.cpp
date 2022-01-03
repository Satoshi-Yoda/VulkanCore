#include "Wing.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ranges>
#include <thread>

using namespace std;
using namespace std::ranges::views;

Wing::Wing(size_t count) {
	start = chrono::steady_clock::now();
	cpuThreads = (count > 0) ? count : thread::hardware_concurrency();
	assert(cpuThreads > 0);

	for (size_t i = 0; i < cpuThreads; i++) {
		specialists.emplace_back(ST_CPU, i + 101, *this);
	}

	ready = chrono::steady_clock::now();
}

Wing::~Wing() {
	finish();

	mtx.lock();
		quitFlag = true;
	mtx.unlock();
	task_cv.notify_all();

	for (auto& specialist : specialists) specialist.thr->join();
}

shared_ptr<Task> Wing::task(const function<void()> func, const set<shared_ptr<Task>> dependencies) {
	shared_ptr<Task> task = make_shared<Task>(ST_CPU, func);
	task->dependencies = dependencies;

	mtx.lock();
		std::erase_if(task->dependencies, [](auto& dependency){
			return dependency->done;
		});

		for (auto& dependency : task->dependencies) {
			dependency->dependants.insert(task);
		}

		if (task->dependencies.empty()) {
			availableTasks.push(task);
			task_cv.notify_one();
		} else {
			blockedTasks.insert(task);
		}
	mtx.unlock();

	return task;
}

shared_ptr<Task> Wing::idleTask(const function<void()> func) {
	shared_ptr<Task> task = make_shared<Task>(ST_CPU, func);
	task->isIdle = true;

	mtx.lock();
		idleTasks.push(task);
		task_cv.notify_one();
	mtx.unlock();

	return task;
}

void Wing::stopIdleTask(const shared_ptr<Task> task) {
	mtx.lock();
		stoppingIdleTasks.insert(task);
	mtx.unlock();
}

void Wing::join() {
	unique_lock<mutex> lock { mtx };
	join_cv.wait(lock, [&]{
		bool done = true;
		done = done && blockedTasks.empty();

		for (size_t i = 0; i < SpecialityCount; i++) {
			done = done && availableTasks.empty();
		}

		for (auto& specialist : specialists) {
			bool busy = (specialist.task.has_value() && (specialist.task.value()->isIdle == false));
			done = done && !busy;
		}

		return done;
	});
}

void Wing::finish() {
	unique_lock<mutex> lock { mtx };

	while (idleTasks.empty() == false) {
		idleTasks.pop();
	}

	join_cv.wait(lock, [&]{
		bool done = true;
		done = done && blockedTasks.empty();
		done = done && availableTasks.empty();

		for (auto& specialist : specialists) {
			bool busy = (specialist.task.has_value());
			done = done && !busy;
		}

		return done;
	});
}

Technician* Wing::findCurrentTechnician() {
	auto id = std::this_thread::get_id();

	for (auto& specialist : specialists) {
		if (specialist.thr->get_id() == id) {
			return &specialist;
		}
	}
	return nullptr;
}

double Wing::initTime() {
	return chrono::duration_cast<chrono::duration<double>>(ready - start).count();
}

double Wing::workTime() {
	return chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - ready).count();
}

pair<size_t, size_t> Wing::specialistsIdRange() {
	set<size_t> ids;
	for (auto& specialist : specialists) {
		ids.insert(specialist.id);
	}

	auto [minId, maxId] = ranges::minmax(ids);
	assert(maxId - minId + 1 == ids.size());
	return make_pair(minId, maxId);
}
