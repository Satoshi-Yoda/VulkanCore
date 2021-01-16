#ifndef TASK_H
#define TASK_H

#include <functional>
#include <memory>
#include <set>

using std::function;
using std::set;
using std::shared_ptr;

enum Speciality {
	ST_CPU,
	ST_GPU,
	ST_PCI,
	ST_HDD,
};

const size_t SpecialityCount = 4;

struct Task {
	Task(Speciality speciality, function<void()> func) : speciality(speciality), func(func) {};
	~Task() {};

	Speciality speciality;
	function<void()> func;
	set<shared_ptr<Task>> dependencies;
	set<shared_ptr<Task>> dependants;
	bool done = false;
};

#endif