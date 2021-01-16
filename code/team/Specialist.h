#ifndef SPECIALIST_H
#define SPECIALIST_H

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "Task.h"
#include "Team.h"

using std::array;
using std::atomic_bool;
using std::mutex;
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

	atomic_bool done = false;
	Speciality speciality;
	size_t id = 0;
	thread* thr;

private:
	Team& team;

};

#endif