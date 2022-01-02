#pragma once

#include <memory>
#include <optional>
#include <thread>

#include "Task.h"
#include "Wing.h"

#include "../core/Rocks.h"

using std::optional;
using std::shared_ptr;
using std::thread;

class Wing;

class Technician {
public:
	Technician(Speciality _speciality, size_t _id, Wing& _wing);
	~Technician();

	Technician(const Technician&)            = delete;
	Technician(Technician&&)                 = delete;
	Technician& operator=(const Technician&) = delete;
	Technician& operator=(Technician&&)      = delete;

	Speciality speciality;
	size_t id = 0;
	thread* thr;
	optional<shared_ptr<Task>> task {};

private:
	Wing& wing;

};
