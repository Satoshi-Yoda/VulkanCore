#pragma once

#include <memory>
#include <optional>
#include <thread>

#include "Task.h"
#include "Team.h"

#include "../core/Rocks.h"

using std::optional;
using std::shared_ptr;
using std::thread;

class Team;

class Specialist {
public:
	Specialist(Speciality _speciality, size_t _id, Team& _team, Rocks* _rocks = nullptr);
	~Specialist();

	Specialist(const Specialist&)            = delete;
	Specialist(Specialist&&)                 = delete;
	Specialist& operator=(const Specialist&) = delete;
	Specialist& operator=(Specialist&&)      = delete;

	void ensureCommandBuffer();
	void flushCommandBuffer();

	Speciality speciality;
	size_t id = 0;
	thread* thr;
	optional<shared_ptr<Task>> task {};

private:
	Team& team;
	Rocks* rocks = nullptr;
	VkCommandBuffer cb = nullptr;

};
