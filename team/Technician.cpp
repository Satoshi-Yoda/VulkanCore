#include "Technician.h"

#include <cassert>
#include <iostream>

using namespace std;

Technician::Technician(Speciality _speciality, size_t _id, Wing& _wing) : speciality(_speciality), id(_id), wing(_wing) {
	this->thr = new thread([this]{
		while (true) {
			bool quit;

			{
				unique_lock<mutex> lock { wing.mtx };
				wing.task_cv.wait(lock, [&]{ return (wing.availableTasks.empty() == false) || (wing.idleTasks.empty() == false) || wing.quitFlag; });
				quit = wing.quitFlag;
				if (wing.availableTasks.empty() == false) {
					task = wing.availableTasks.front();
					wing.availableTasks.pop();
				} else if (wing.idleTasks.empty() == false) {
					auto candidate = wing.idleTasks.front();
					wing.idleTasks.pop();
					if (wing.stoppingIdleTasks.contains(candidate) || quit) {
						wing.stoppingIdleTasks.erase(candidate);
					} else {
						task = candidate;
						wing.idleTasks.push(candidate);
					}
				}
			}

			if (task.has_value()) {
				auto& taskPtr = task.value();
				if (taskPtr->func != nullptr) {
					assert(speciality != ST_GPU);
					taskPtr->func();
				}

				wing.mtx.lock();
					taskPtr->done = true;

					for (auto& dependant : taskPtr->dependants) {
						dependant->dependencies.erase(taskPtr);
						if (dependant->dependencies.empty()) {
							wing.blockedTasks.erase(dependant);
							wing.availableTasks.push(dependant);
							wing.task_cv.notify_one();
						}
					}

					task.reset();
				wing.mtx.unlock();
			}

			wing.join_cv.notify_one();
			wing.finish_cv.notify_one();

			if (quit) {
				break;
			}
		}
	});
}

Technician::~Technician() {
	// TODO how about delete this->thr?
}
