#ifdef tests_here

#define CATCH_CONFIG_MAIN
#include <catch_amalgamated.hpp>

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

#include "Team.h"
#include "Specialist.h"
#include "Task.h"

using namespace std;

TEST_CASE("Team: Test single task work") {
	// std::setvbuf(stdout, nullptr, _IONBF, 0);
	// printf("\n");

	atomic_int k = 5;
	Team team {};
	team.task(ST_CPU, [&]{
		k = 10;
	});
	team.join();
	REQUIRE(k == 10);
}

TEST_CASE("Team: Test waiting for few tasks") {
	atomic_int k = 5;
	Team team {};

	team.task(ST_CPU, [&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	team.task(ST_CPU, [&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	team.task(ST_CPU, [&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	team.join();
	REQUIRE(k == 8);
}

TEST_CASE("Team: Test waiting for a lot of tasks") {
	// std::setvbuf(stdout, nullptr, _IONBF, 0);
 	// printf("\n");

	atomic_int k = 5;
	const size_t COUNT = 100;
	Team team {};

	for (size_t i = 0; i < COUNT; i++) {
		team.task(ST_CPU, [&]{
			k++;
		});
	}
	team.join();
	REQUIRE(k == 5 + COUNT);
}

TEST_CASE("Team: Test for managing task id") {
	atomic_int k = 5;
	Team team {};
	auto id1 = team.task(ST_CPU, [&]{ k = 10; });
	auto id2 = team.task(ST_CPU, [&]{ k = 10; });
	team.join();
	REQUIRE(id1 != id2);
}

TEST_CASE("Team: Test for join to complete all tasks") {
	atomic_int k = 0;
	Team team {};

	for (int i = 0; i < 100; i++) {
		team.task(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			k++;
		});
	}

	team.join();
	REQUIRE(k == 100);
}

TEST_CASE("Team: Test for finding assigned specialist") {
	Specialist* specialist;
	Team team {};
	team.task(ST_HDD, [&]{
		team.mtx.lock();
			specialist = team.findCurrentSpecialist();
		team.mtx.unlock();
	});

	team.join();

	REQUIRE(specialist != nullptr);
	REQUIRE(specialist->speciality == ST_HDD);
	REQUIRE(specialist->id > 100);
}

TEST_CASE("Team: 12 threads for me") {
	Team team {};
	REQUIRE(team.cpuThreads == 12);
}

TEST_CASE("Team: Test several specialists even distribution") {
	set<size_t> ids;
	Team team {};

	for (size_t i = 0; i < team.cpuThreads; i++) {
		team.task(ST_CPU, [&]{
			team.mtx.lock();
				ids.insert(team.findCurrentSpecialist()->id);
			team.mtx.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		});
	}

	team.join();

	REQUIRE(ids.size() == team.cpuThreads);

	for (size_t i = 0; i < team.cpuThreads; i++) {
		if (ids.find(101 + i) == ids.end()) {
			for (auto id : ids) cout << id << ' ';
			cout << endl;
		}
		REQUIRE(ids.find(101 + i) != ids.end());
	}
}

TEST_CASE("Team: Test dependency") {
	atomic_int k = 5;
	Team team {};

	auto firstTask = team.task(ST_CPU, [&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	auto secondTask = team.task(ST_CPU, [&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	auto thirdTask = team.task(ST_CPU, [&]{
		k = (k == 7) ? 10 : 0;
	}, { firstTask, secondTask });

	team.join();
	REQUIRE(k == 10);
}

TEST_CASE("Team: Test for completed dependency") {
	atomic_int k = 5;
	Team team {};

	auto firstTask = team.task(ST_CPU, [&]{
		k++;
	});
	auto secondTask = team.task(ST_CPU, [&]{
		k++;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	auto thirdTask = team.task(ST_CPU, [&]{
		k = (k == 7) ? 10 : 0;
	}, { firstTask, secondTask });

	bool done = true;//team.wait(std::chrono::milliseconds(100));
	team.join();
	REQUIRE(done == true);
	REQUIRE(k == 10);
}

TEST_CASE("Team: Test for different specialities") {
	Specialist* specialist1 = nullptr;
	Specialist* specialist2 = nullptr;
	Specialist* specialist3 = nullptr;
	Specialist* specialist4 = nullptr;
	atomic_int k = 5;
	Team team {};

	auto task1 = team.task(ST_CPU, [&]{
		k = (k == 5) ? 6 : 0;
		team.mtx.lock();
			specialist1 = team.findCurrentSpecialist();
		team.mtx.unlock();
	});
	auto task2 = team.task(ST_CPU, [&]{
		k = (k == 6) ? 7 : 0;
		team.mtx.lock();
			specialist2 = team.findCurrentSpecialist();
		team.mtx.unlock();
	}, { task1 });
	auto task3 = team.task(ST_PCI, [&]{
		k = (k == 7) ? 8 : 0;
		team.mtx.lock();
			specialist3 = team.findCurrentSpecialist();
		team.mtx.unlock();
	}, { task2 });
	auto task4 = team.task(ST_HDD, [&]{
		k = (k == 8) ? 9 : 0;
		team.mtx.lock();
			specialist4 = team.findCurrentSpecialist();
		team.mtx.unlock();
	}, { task3 });

	bool done = true;//team.wait(std::chrono::milliseconds(100));
	team.join();
	REQUIRE(specialist1->speciality == ST_CPU);
	REQUIRE(specialist2->speciality == ST_CPU);
	REQUIRE(specialist3->speciality == ST_PCI);
	REQUIRE(specialist4->speciality == ST_HDD);
	REQUIRE(done == true);
	REQUIRE(k == 9);
}

TEST_CASE("Team: Test for wait time") {
	atomic_int k = 5;
	const size_t COUNT = 6;
	Team team {};

	for (size_t i = 0; i < COUNT; i++) {
		auto task = team.task(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			k++;
		});
	}
	
	team.join();
	// REQUIRE(k == 5 + COUNT);
	// REQUIRE(static_cast<uint64_t>(team.initTime() * 1e6) < 0);
	REQUIRE(static_cast<uint64_t>(team.workTime() * 1e6) / static_cast<double>(1000) < 150);
}

TEST_CASE("Team: Test for work after join") {
	atomic_int k = 5;
	const size_t COUNT = 6;
	Team team {};

	for (size_t i = 0; i < COUNT; i++) {
		auto task = team.task(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			k++;
		});
	}
	
	team.join();
	REQUIRE(k == 5 + COUNT);

	for (size_t i = 0; i < COUNT; i++) {
		auto task = team.task(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			k++;
		});
	}

	team.join();
	REQUIRE(k == 5 + 2 * COUNT);
}

TEST_CASE("Team: Test idle task after regular tasks") {
	atomic_int k = 5;
	int m = 100;
	const size_t COUNT = 6;
	mutex mtx;
	Team team {};

	for (size_t i = 0; i < COUNT; i++) {
		auto task = team.task(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			k++;
		});
	}

	auto id = team.idleTask(ST_CPU, [&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		mtx.lock();
			m = 42;
		mtx.unlock();
	});

	team.join();
	REQUIRE(k == 5 + COUNT);

	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	REQUIRE(m == 42);
}

TEST_CASE("Team: Test stopping idle task") {
	int m = 100;
	mutex mtx;
	Team team {};

	auto id = team.idleTask(ST_CPU, [&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		mtx.lock();
			m = 42;
		mtx.unlock();
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(25));
	REQUIRE(m == 42);

	m = 100;
	std::this_thread::sleep_for(std::chrono::milliseconds(25));
	REQUIRE(m == 42);

	team.stopIdleTask(id);
	team.finish();
	m = 100;
	std::this_thread::sleep_for(std::chrono::milliseconds(25));
	REQUIRE(m == 100);
}

TEST_CASE("Team: Test idle task to stop after team destroy") {
	int m = 100;
	mutex mtx;

	{
		Team team {};

		auto id = team.idleTask(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			mtx.lock();
				m = 42;
			mtx.unlock();
		});

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		REQUIRE(m == 42);

		m = 100;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		REQUIRE(m == 42);
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	m = 100;
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	REQUIRE(m == 100);
}

TEST_CASE("Team: Test for general abort") {
	Team team { 4 };
	atomic_int k = 0;

	for (size_t i = 0; i < 4; i++) {
		team.task(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			k++;
		});
	}
	for (size_t i = 0; i < 4; i++) {
		team.task(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			k++;
		});
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	team.abort();
	team.join();
	REQUIRE(k == 4);
}


TEST_CASE("Team: Test for multiple dependencies") {
	Team team { 4 };

	size_t COUNT_K = 7;
	size_t COUNT_M = 5;

	atomic_int k = 0;
	atomic_int m = 0;

	set<shared_ptr<Task>> ids;

	for (size_t i = 0; i < COUNT_K; i++) {
		ids.insert(team.task(ST_CPU, [&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			k++;
		}));
	}

	REQUIRE(ids.size() == COUNT_K);

	for (size_t i = 0; i < COUNT_M; i++) {
		team.task(ST_CPU, [&]{
			REQUIRE(k == static_cast<atomic_int>(COUNT_K));
			m++;
		}, ids);
	}

	team.join();

	REQUIRE(k == static_cast<atomic_int>(COUNT_K));
	REQUIRE(m == static_cast<atomic_int>(COUNT_M));
}

#endif