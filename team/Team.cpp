#include "Team.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ranges>
#include <thread>

using namespace std;
using namespace std::ranges::views;

Team::Team() {
	start = chrono::steady_clock::now();
	cpuThreads = thread::hardware_concurrency();

	// TODO maybe do project-separate groups initialization

	for (size_t i = 0; i < cpuThreads; i++) {
		specialists.emplace_back(ST_CPU, i + 101, *this);
	}
	// for (size_t i = 0; i < 1; i++) {
	// 	specialists.emplace_back(ST_GPU, i + 101, *this);
	// }
	for (size_t i = 0; i < 1; i++) {
		specialists.emplace_back(ST_PCI, i + 101, *this);
	}
	for (size_t i = 0; i < 32; i++) {
		specialists.emplace_back(ST_HDD, i + 101, *this);
	}

	ready = chrono::steady_clock::now();
}

Team::~Team() {
	finish();

	mtx.lock();
		quitFlag = true;
	mtx.unlock();
	for (auto& cv : cvs) cv.notify_all();

	for (auto& specialist : specialists) specialist.thr->join();
}

void Team::initGpuSpecialists(Rocks& rocks) {
	for (size_t i = 0; i < 1; i++) {
		specialists.emplace_back(ST_GPU, i + 101, *this, &rocks);
	}
}

shared_ptr<Task> Team::task(const Speciality speciality, const function<void()> func, const set<shared_ptr<Task>> dependencies) {
	assert(speciality != ST_GPU); // TODO remove later

	shared_ptr<Task> task = make_shared<Task>(speciality, func);
	task->dependencies = dependencies;

	mtx.lock();
		std::erase_if(task->dependencies, [](auto& dependency){
			return dependency->done;
		});

		for (auto& dependency : task->dependencies) {
			dependency->dependants.insert(task);
		}

		if (task->dependencies.empty()) {
			size_t index = static_cast<size_t>(speciality);
			availableTasks[index].push(task);
			cvs[index].notify_one();
		} else {
			blockedTasks.insert(task);
		}
	mtx.unlock();

	return task;
}

shared_ptr<Task> Team::gpuTask(const function<void(VkCommandBuffer)> func, const set<shared_ptr<Task>> dependencies) {
	assert(count_if(specialists.begin(), specialists.end(), [](auto& s){ return s.speciality == ST_GPU; }) > 0); // TODO maybe not, maybe you can create specialist(s) after creating task

	// for (auto& specialist : specialists | filter([](auto& s){ return s.speciality == ST_GPU; })) {
	// 	specialist.ensureCommandBuffer();
	// }

	shared_ptr<Task> task = make_shared<Task>(ST_GPU, func);
	task->dependencies = dependencies;

	mtx.lock();
		std::erase_if(task->dependencies, [](auto& dependency){
			return dependency->done;
		});

		for (auto& dependency : task->dependencies) {
			dependency->dependants.insert(task);
		}

		if (task->dependencies.empty()) {
			size_t index = static_cast<size_t>(ST_GPU);
			availableTasks[index].push(task);
			cvs[index].notify_one();
		} else {
			blockedTasks.insert(task);
		}
	mtx.unlock();

	return task;
}

shared_ptr<Task> Team::idleTask(const Speciality speciality, const function<void()> func) {
	assert(speciality != ST_GPU); // TODO remove later

	shared_ptr<Task> task = make_shared<Task>(speciality, func);
	task->isIdle = true;

	mtx.lock();
		size_t index = static_cast<size_t>(speciality);
		idleTasks[index].push(task);
		cvs[index].notify_one();
	mtx.unlock();

	return task;
}

void Team::stopIdleTask(const shared_ptr<Task> task) {
	assert(task->speciality != ST_GPU); // TODO remove later

	mtx.lock();
		size_t index = static_cast<size_t>(task->speciality);
		stoppingIdleTasks[index].insert(task);
	mtx.unlock();
}

void Team::join() {
	unique_lock<mutex> lock { mtx };
	join_cv.wait(lock, [&]{
		bool done = true;
		done = done && blockedTasks.empty();

		for (size_t i = 0; i < SpecialityCount; i++) {
			done = done && availableTasks[i].empty();
		}

		for (auto& specialist : specialists) {
			bool busy = (specialist.task.has_value() && (specialist.task.value()->isIdle == false));
			done = done && !busy;
		}

		return done;
	});

	// for (auto& specialist : specialists | filter([](auto& s){ return s.speciality == ST_GPU; })) {
	// 	specialist.flushCommandBuffer();
	// }
}

void Team::finish() {
	unique_lock<mutex> lock { mtx };

	for (size_t i = 0; i < SpecialityCount; i++) {
		while (idleTasks[i].empty() == false) {
			idleTasks[i].pop();
		}
	}

	join_cv.wait(lock, [&]{
		bool done = true;
		done = done && blockedTasks.empty();

		for (size_t i = 0; i < SpecialityCount; i++) {
			done = done && availableTasks[i].empty();
		}

		for (auto& specialist : specialists) {
			bool busy = (specialist.task.has_value());
			done = done && !busy;
		}

		return done;
	});
}

// bool Team::wait(std::chrono::milliseconds time) {
// 	auto start = std::chrono::steady_clock::now();

// 	bool noBlocked = false;

// 	while (std::chrono::steady_clock::now() < start + time) {
// 		bool done = true;
// 		mtx.lock();
// 			done = done && blockedTasks.empty();
// 			for (size_t i = 0; i < SpecialityCount; i++) {
// 				done = done && availableTasks[i].empty();
// 			}
// 		mtx.unlock();

// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));

// 		if (done) {
// 			noBlocked = true;
// 			break;
// 		}
// 	}

// 	mtx.lock();
// 		quitFlag = true;
// 	mtx.unlock();
// 	for (auto& cv : cvs) cv.notify_all();

// 	bool noInProgress = false;

// 	while (std::chrono::steady_clock::now() < start + time * 2) {
// 		bool done = true;
// 		for (auto& specialist : specialists) {
// 			done = done && specialist.done;
// 		}

// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));

// 		if (done) {
// 			noInProgress = true;
// 			break;
// 		}
// 	}

// 	return noBlocked && noInProgress;
// }

Specialist* Team::findCurrentSpecialist() {
	auto id = std::this_thread::get_id();

	for (auto& specialist : specialists) {
		if (specialist.thr->get_id() == id) {
			return &specialist;
		}
	}
	return nullptr;
}

double Team::initTime() {
	return chrono::duration_cast<chrono::duration<double>>(ready - start).count();
}

double Team::workTime() {
	return chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - ready).count();
}
