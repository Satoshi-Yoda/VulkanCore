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
				// printf("Specialist %d-%d start waiting task\n", speciality, id);
				team.cvs[index].wait(lock, [&]{ return (team.availableTasks[index].empty() == false) || team.quitFlag; });
				quit = team.quitFlag;
				// printf("Specialist %d-%d end waiting task\n", speciality, id);
				if (team.availableTasks[index].empty() == false) {
					// size_t sizeBefore = team.availableTasks[index].size();
					task = team.availableTasks[index].front();
					team.availableTasks[index].pop();
					// task = *team.availableTasks[index].begin();
					// team.availableTasks[index].erase(team.availableTasks[index].begin());
					// size_t sizeAfter = team.availableTasks[index].size();
					// printf("Specialist %d-%d availableBucket.size(): %lld -> %lld\n", speciality, id, sizeBefore, sizeAfter);
				} else if (team.idleTasks[index].empty() == false) {
					auto candidate = team.idleTasks[index].front();
					printf("Found idle task candidate\n");
					team.idleTasks[index].pop();
					if (team.stoppingIdleTasks[index].contains(candidate)) {
						team.stoppingIdleTasks[index].erase(candidate);
						printf("Idle task marked for stop\n");
					} else {
						task = candidate;
						team.idleTasks[index].push(candidate);
					}
				}
			}

			if (task.has_value()) {
				auto& taskPtr = task.value();
				// printf("Specialist %d-%d starting task: %lld\n", speciality, id, taskPtr.get());
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

					// printf("Specialist %d-%d task %lld completed, found %lld dependants\n", speciality, id, taskPtr.get(), taskPtr->dependants.size());
					for (auto& dependant : taskPtr->dependants) {
						dependant->dependencies.erase(taskPtr);
						if (dependant->dependencies.empty()) {
							team.blockedTasks.erase(dependant);
							size_t index = static_cast<size_t>(dependant->speciality);
							team.availableTasks[index].push(dependant);
							team.cvs[index].notify_one();
							// printf("Specialist %d-%d moved task to availables: %lld\n", speciality, id, dependant.get());
						}
					}

					task.reset();
				team.mutex.unlock();
			}

			team.ready_cv.notify_one();

			if (quit) {
				// printf("Specialist %d quit work\n", id);
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
