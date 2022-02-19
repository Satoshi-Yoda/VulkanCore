#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <ranges>
#include <set>
#include <thread>
#include <utility>
#include <vector>

using std::array;
using std::condition_variable;
using std::function;
using std::list;
using std::mutex;
using std::optional;
using std::pair;
using std::queue;
using std::set;
using std::shared_ptr;
using std::thread;
using std::unique_lock;
using std::vector;

struct Errand {
	Errand(function<void()> func) : func(func) {};
	~Errand() {};

	function<void()> func;
	set<shared_ptr<Errand>> dependencies;
	set<shared_ptr<Errand>> dependants;
	// bool isIdle = false;
	bool done = false;
};

struct Technician {
	size_t id = 0;
	thread* thr;
	optional<shared_ptr<Errand>> task {};
};

// template <typename T>
class Wing {
public:
	Wing(size_t count = 0) {
		start = std::chrono::steady_clock::now();
		cpuThreads = (count > 0) ? count : thread::hardware_concurrency();
		assert(cpuThreads > 0);

		technicians.reserve(cpuThreads);

		for (size_t i = 0; i < cpuThreads; i++) {
			technicians.emplace_back();

			size_t index = technicians.size() - 1;

			technicians.back().thr = new thread([this, index]{
				while (true) {
					bool quit;

					Technician& t = technicians[index];

					{
						unique_lock<mutex> lock { mtx };
						task_cv.wait(lock, [&]{ return (availableTasks.empty() == false) || (idleTasks.empty() == false) || quitFlag; });
						quit = quitFlag;
						if (availableTasks.empty() == false) {
							t.task = availableTasks.front();
							availableTasks.pop();
						} else if (idleTasks.empty() == false) {
							auto candidate = idleTasks.front();
							idleTasks.pop();
							if (stoppingIdleTasks.contains(candidate) || quit) {
								stoppingIdleTasks.erase(candidate);
							} else {
								t.task = candidate;
								idleTasks.push(candidate);
							}
						}
					}

					if (t.task.has_value()) {
						auto& taskPtr = t.task.value();
						if (taskPtr->func != nullptr) {
							taskPtr->func();
						}

						mtx.lock();
							taskPtr->done = true;

							for (auto& dependant : taskPtr->dependants) {
								dependant->dependencies.erase(taskPtr);
								if (dependant->dependencies.empty()) {
									blockedTasks.erase(dependant);
									availableTasks.push(dependant);
									task_cv.notify_one();
								}
							}

							t.task.reset();
						mtx.unlock();
					}

					join_cv.notify_one();
					finish_cv.notify_one();

					if (quit) {
						break;
					}
				}
			});
		}

		ready = std::chrono::steady_clock::now();
	}

	~Wing() {
		finish();

		mtx.lock();
			quitFlag = true;
		mtx.unlock();
		task_cv.notify_all();

		for (auto& technician : technicians) technician.thr->join();
	}

	shared_ptr<Errand> task(const function<void()> func, const set<shared_ptr<Errand>> dependencies = set<shared_ptr<Errand>>()) {
		shared_ptr<Errand> task = make_shared<Errand>(func);
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

	void join() {
		unique_lock<mutex> lock { mtx };
		join_cv.wait(lock, [&]{
			bool done = true;
			done = done && blockedTasks.empty();
			done = done && availableTasks.empty();

			for (auto& technician : technicians) {
				// bool busy = (technician.task.has_value() && (technician.task.value()->isIdle == false));
				bool busy = technician.task.has_value();
				done = done && !busy;
			}

			return done;
		});
	}

	void finish() {
		unique_lock<mutex> lock { mtx };

		while (idleTasks.empty() == false) {
			idleTasks.pop();
		}

		join_cv.wait(lock, [&]{
			bool done = true;
			done = done && blockedTasks.empty();
			done = done && availableTasks.empty();

			for (auto& technician : technicians) {
				bool busy = (technician.task.has_value());
				done = done && !busy;
			}

			return done;
		});
	}

	// Technician* findCurrentTechnician() {
	// 	auto id = std::this_thread::get_id();

	// 	for (auto& technician : technicians) {
	// 		if (technician.thr->get_id() == id) {
	// 			return &technician;
	// 		}
	// 	}
	// 	return nullptr;
	// }

	double initTime() {
		return std::chrono::duration_cast<std::chrono::duration<double>>(ready - start).count();
	}

	double workTime() {
		return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - ready).count();
	}

	pair<size_t, size_t> specialistsIdRange() {
		set<size_t> ids;
		for (auto& technician : technicians) {
			ids.insert(technician.id);
		}

		auto [minId, maxId] = std::ranges::minmax(ids);
		assert(maxId - minId + 1 == ids.size());
		return std::make_pair(minId, maxId);
	}

	uint32_t cpuThreads;

	mutex mtx;
	condition_variable task_cv;
	condition_variable join_cv;
	condition_variable finish_cv;

	queue<shared_ptr<Errand>> idleTasks;
	set<shared_ptr<Errand>> stoppingIdleTasks;
	queue<shared_ptr<Errand>> availableTasks;
	set<shared_ptr<Errand>> blockedTasks;
	bool quitFlag = false;

private:
	// T toolkit;
	vector<Technician> technicians;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> ready;

};
