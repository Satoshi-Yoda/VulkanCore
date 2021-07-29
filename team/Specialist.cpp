#include "Specialist.h"

#include <cassert>
#include <iostream>

using namespace std;

Specialist::Specialist(Speciality _speciality, size_t _id, Team& _team, Rocks* _rocks) : speciality(_speciality), id(_id), team(_team), rocks(_rocks) {
	assert((_speciality == ST_GPU) == (rocks != nullptr));

	this->thr = new thread([this]{
		while (true) {
			size_t index = static_cast<size_t>(speciality);
			bool quit;

			{
				unique_lock<mutex> lock { team.mutex };
				team.cvs[index].wait(lock, [&]{ return (team.availableTasks[index].empty() == false) || (team.idleTasks[index].empty() == false) || team.quitFlag; });
				quit = team.quitFlag;
				if (team.availableTasks[index].empty() == false) {
					task = team.availableTasks[index].front();
					team.availableTasks[index].pop();
				} else if (team.idleTasks[index].empty() == false) {
					auto candidate = team.idleTasks[index].front();
					team.idleTasks[index].pop();
					if (team.stoppingIdleTasks[index].contains(candidate) || quit) {
						team.stoppingIdleTasks[index].erase(candidate);
					} else {
						task = candidate;
						team.idleTasks[index].push(candidate);
					}
				}
			}

			if (task.has_value()) {
				auto& taskPtr = task.value();
				if (taskPtr->func != nullptr) {
					assert(speciality != ST_GPU);
					taskPtr->func();
				} else if (taskPtr->cbFunc != nullptr) {
					assert(speciality == ST_GPU);
					assert(rocks != nullptr);
					// assert(cb != nullptr);
					VkCommandBuffer cb = rocks->beginSingleTimeCommands();
					taskPtr->cbFunc(cb);
					rocks->endSingleTimeCommands(cb);
					cb = nullptr;
				}

				team.mutex.lock();
					taskPtr->done = true;

					for (auto& dependant : taskPtr->dependants) {
						dependant->dependencies.erase(taskPtr);
						if (dependant->dependencies.empty()) {
							team.blockedTasks.erase(dependant);
							size_t dependantIndex = static_cast<size_t>(dependant->speciality);
							team.availableTasks[dependantIndex].push(dependant);
							team.cvs[dependantIndex].notify_one();
						}
					}

					task.reset();
				team.mutex.unlock();
			}

			team.ready_cv.notify_one();

			if (quit) {
				break;
			}
		}
	});
}

Specialist::~Specialist() {}

// void Specialist::ensureCommandBuffer() {
// 	assert(speciality == ST_GPU);
// 	assert(rocks != nullptr);

// 	if (cb == nullptr) {
// 		cb = rocks->beginSingleTimeCommands();
// 	}
// }

// void Specialist::flushCommandBuffer() {
// 	assert(speciality == ST_GPU);
// 	assert(rocks != nullptr);

// 	if (cb != nullptr) {
// 		rocks->endSingleTimeCommands(cb);
// 		cb = nullptr;
// 	}
// }
