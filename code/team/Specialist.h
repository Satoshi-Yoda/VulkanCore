#pragma once

#include <memory>
#include <optional>
#include <thread>

#include "Task.h"
#include "Team.h"

using std::optional;
using std::shared_ptr;
using std::thread;

class Team;

class Specialist {
public:
	Specialist(Speciality _speciality, size_t _id, Team& _team);
	~Specialist();

	Specialist(const Specialist&)            = delete;
	Specialist(Specialist&&)                 = delete;
	Specialist& operator=(const Specialist&) = delete;
	Specialist& operator=(Specialist&&)      = delete;

	Speciality speciality;
	size_t id = 0;
	thread* thr;
	optional<shared_ptr<Task>> task {};

private:
	Team& team;

};
