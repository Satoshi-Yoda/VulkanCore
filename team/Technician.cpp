#include "Technician.h"

#include <cassert>
#include <iostream>

using namespace std;

Technician::Technician(Speciality _speciality, size_t _id, Wing& _wing) : speciality(_speciality), id(_id), wing(_wing) {
	this->thr = new thread([this]{
		while (true) {
			size_t index = static_cast<size_t>(speciality);
			bool quit;

			{
				unique_lock<mutex> lock { wing.mtx };
				wing.cvs[index].wait(lock, [&]{ return (wing.availableTasks[index].empty() == false) || (wing.idleTasks[index].empty() == false) || wing.quitFlag; });
				quit = wing.quitFlag;
				if (wing.availableTasks[index].empty() == false) {
					task = wing.availableTasks[index].front();
					wing.availableTasks[index].pop();
				} else if (wing.idleTasks[index].empty() == false) {
					auto candidate = wing.idleTasks[index].front();
					wing.idleTasks[index].pop();
					if (wing.stoppingIdleTasks[index].contains(candidate) || quit) {
						wing.stoppingIdleTasks[index].erase(candidate);
					} else {
						task = candidate;
						wing.idleTasks[index].push(candidate);
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
							size_t dependantIndex = static_cast<size_t>(dependant->speciality);
							wing.availableTasks[dependantIndex].push(dependant);
							wing.cvs[dependantIndex].notify_one();
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
