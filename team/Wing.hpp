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

template <typename T>
struct Errand {
	Errand(function<void(T)> func) : func(func) {};
	~Errand() {};

	function<void(T)> func;
	set<shared_ptr<Errand<T>>> dependencies;
	set<shared_ptr<Errand<T>>> dependants;
	// bool isIdle = false;
	bool done = false;
};

template <typename T>
struct Technician {
	size_t id = 0;
	thread* thr;
	optional<shared_ptr<Errand<T>>> errand {};
};

template <typename T>
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

					Technician<T>& t = technicians[index];

					{
						unique_lock<mutex> lock { mtx };
						errand_cv.wait(lock, [&]{ return (availableTasks.empty() == false) || (idleTasks.empty() == false) || quitFlag; });
						quit = quitFlag;
						if (availableTasks.empty() == false) {
							t.errand = availableTasks.front();
							availableTasks.pop();
						} else if (idleTasks.empty() == false) {
							auto candidate = idleTasks.front();
							idleTasks.pop();
							if (stoppingIdleTasks.contains(candidate) || quit) {
								stoppingIdleTasks.erase(candidate);
							} else {
								t.errand = candidate;
								idleTasks.push(candidate);
							}
						}
					}

					if (t.errand.has_value()) {
						auto& errandPtr = t.errand.value();
						if (errandPtr->func != nullptr) {
							errandPtr->func(T{}); // TODO use technician's T
						}

						mtx.lock();
							errandPtr->done = true;

							for (auto& dependant : errandPtr->dependants) {
								dependant->dependencies.erase(errandPtr);
								if (dependant->dependencies.empty()) {
									blockedTasks.erase(dependant);
									availableTasks.push(dependant);
									errand_cv.notify_one();
								}
							}

							t.errand.reset();
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
		errand_cv.notify_all();

		for (auto& technician : technicians) technician.thr->join();
	}

	shared_ptr<Errand<T>> errand(const function<void(T)> func, const set<shared_ptr<Errand<T>>> dependencies = set<shared_ptr<Errand<T>>>()) {
		shared_ptr<Errand<T>> errand = make_shared<Errand<T>>(func);
		errand->dependencies = dependencies;

		mtx.lock();
			std::erase_if(errand->dependencies, [](auto& dependency){
				return dependency->done;
			});

			for (auto& dependency : errand->dependencies) {
				dependency->dependants.insert(errand);
			}

			if (errand->dependencies.empty()) {
				availableTasks.push(errand);
				errand_cv.notify_one();
			} else {
				blockedTasks.insert(errand);
			}
		mtx.unlock();

		return errand;
	}

	void join() {
		unique_lock<mutex> lock { mtx };
		join_cv.wait(lock, [&]{
			bool done = true;
			done = done && blockedTasks.empty();
			done = done && availableTasks.empty();

			for (auto& technician : technicians) {
				// bool busy = (technician.errand.has_value() && (technician.errand.value()->isIdle == false));
				bool busy = technician.errand.has_value();
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
				bool busy = (technician.errand.has_value());
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
	condition_variable errand_cv;
	condition_variable join_cv;
	condition_variable finish_cv;

	queue<shared_ptr<Errand<T>>> idleTasks;
	set<shared_ptr<Errand<T>>> stoppingIdleTasks;
	queue<shared_ptr<Errand<T>>> availableTasks;
	set<shared_ptr<Errand<T>>> blockedTasks;
	bool quitFlag = false;

private:
	// T toolkit;
	vector<Technician<T>> technicians;
	std::chrono::time_point<std::chrono::steady_clock> start;
	std::chrono::time_point<std::chrono::steady_clock> ready;

};
