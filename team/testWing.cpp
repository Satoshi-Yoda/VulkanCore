#ifdef tests_here

#define CATCH_CONFIG_MAIN
#include <catch_amalgamated.hpp>

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

#include "Wing.h"
#include "Technician.h"
#include "Task.h"

using namespace std;

TEST_CASE("Wing: Test single task work") {
	atomic_int k = 5;
	Wing wing {};
	wing.task([&]{
		k = 10;
	});
	wing.join();
	REQUIRE(k == 10);
}

// TEST_CASE("Test waiting for few tasks") {
// 	atomic_int k = 5;
// 	Wing wing {};

// 	wing.task(ST_CPU, [&]{
// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 		k++;
// 	});
// 	wing.task(ST_CPU, [&]{
// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 		k++;
// 	});
// 	wing.task(ST_CPU, [&]{
// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 		k++;
// 	});
// 	wing.join();
// 	REQUIRE(k == 8);
// }

// TEST_CASE("Test waiting for a lot of tasks") {
// 	// std::setvbuf(stdout, nullptr, _IONBF, 0);
//  	// printf("\n");

// 	atomic_int k = 5;
// 	const size_t COUNT = 100;
// 	Wing wing {};

// 	for (size_t i = 0; i < COUNT; i++) {
// 		wing.task(ST_CPU, [&]{
// 			k++;
// 		});
// 	}
// 	wing.join();
// 	REQUIRE(k == 5 + COUNT);
// }

// TEST_CASE("Test for managing task id") {
// 	atomic_int k = 5;
// 	Wing wing {};
// 	auto id1 = wing.task(ST_CPU, [&]{ k = 10; });
// 	auto id2 = wing.task(ST_CPU, [&]{ k = 10; });
// 	wing.join();
// 	REQUIRE(id1 != id2);
// }

// TEST_CASE("Test for join to complete all tasks") {
// 	atomic_int k = 0;
// 	Wing wing {};

// 	for (int i = 0; i < 100; i++) {
// 		wing.task(ST_CPU, [&]{
// 			std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 			k++;
// 		});
// 	}

// 	wing.join();
// 	REQUIRE(k == 100);
// }

// TEST_CASE("Test for finding assigned specialist") {
// 	Technician* specialist;
// 	Wing wing {};
// 	wing.task(ST_HDD, [&]{
// 		wing.mtx.lock();
// 			specialist = wing.findCurrentTechnician();
// 		wing.mtx.unlock();
// 	});

// 	wing.join();

// 	REQUIRE(specialist != nullptr);
// 	REQUIRE(specialist->speciality == ST_HDD);
// 	REQUIRE(specialist->id > 100);
// }

// TEST_CASE("12 threads for me") {
// 	Wing wing {};
// 	REQUIRE(wing.cpuThreads == 12);
// }

// TEST_CASE("Test several specialists even distribution") {
// 	set<size_t> ids;
// 	Wing wing {};

// 	for (size_t i = 0; i < wing.cpuThreads; i++) {
// 		wing.task(ST_CPU, [&]{
// 			wing.mtx.lock();
// 				ids.insert(wing.findCurrentTechnician()->id);
// 			wing.mtx.unlock();
// 			std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 		});
// 	}

// 	wing.join();

// 	REQUIRE(ids.size() == wing.cpuThreads);

// 	for (size_t i = 0; i < wing.cpuThreads; i++) {
// 		if (ids.find(101 + i) == ids.end()) {
// 			for (auto id : ids) cout << id << ' ';
// 			cout << endl;
// 		}
// 		REQUIRE(ids.find(101 + i) != ids.end());
// 	}
// }

// TEST_CASE("Test dependency") {
// 	atomic_int k = 5;
// 	Wing wing {};

// 	auto firstTask = wing.task(ST_CPU, [&]{
// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 		k++;
// 	});
// 	auto secondTask = wing.task(ST_CPU, [&]{
// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 		k++;
// 	});
// 	auto thirdTask = wing.task(ST_CPU, [&]{
// 		k = (k == 7) ? 10 : 0;
// 	}, { firstTask, secondTask });

// 	wing.join();
// 	REQUIRE(k == 10);
// }

// TEST_CASE("Test for completed dependency") {
// 	atomic_int k = 5;
// 	Wing wing {};

// 	auto firstTask = wing.task(ST_CPU, [&]{
// 		k++;
// 	});
// 	auto secondTask = wing.task(ST_CPU, [&]{
// 		k++;
// 	});

// 	std::this_thread::sleep_for(std::chrono::milliseconds(10));

// 	auto thirdTask = wing.task(ST_CPU, [&]{
// 		k = (k == 7) ? 10 : 0;
// 	}, { firstTask, secondTask });

// 	bool done = true;//wing.wait(std::chrono::milliseconds(100));
// 	wing.join();
// 	REQUIRE(done == true);
// 	REQUIRE(k == 10);
// }

// TEST_CASE("Test for different specialities") {
// 	Technician* specialist1 = nullptr;
// 	Technician* specialist2 = nullptr;
// 	Technician* specialist3 = nullptr;
// 	Technician* specialist4 = nullptr;
// 	atomic_int k = 5;
// 	Wing wing {};

// 	auto task1 = wing.task(ST_CPU, [&]{
// 		k = (k == 5) ? 6 : 0;
// 		wing.mtx.lock();
// 			specialist1 = wing.findCurrentTechnician();
// 		wing.mtx.unlock();
// 	});
// 	auto task2 = wing.task(ST_CPU, [&]{
// 		k = (k == 6) ? 7 : 0;
// 		wing.mtx.lock();
// 			specialist2 = wing.findCurrentTechnician();
// 		wing.mtx.unlock();
// 	}, { task1 });
// 	auto task3 = wing.task(ST_PCI, [&]{
// 		k = (k == 7) ? 8 : 0;
// 		wing.mtx.lock();
// 			specialist3 = wing.findCurrentTechnician();
// 		wing.mtx.unlock();
// 	}, { task2 });
// 	auto task4 = wing.task(ST_HDD, [&]{
// 		k = (k == 8) ? 9 : 0;
// 		wing.mtx.lock();
// 			specialist4 = wing.findCurrentTechnician();
// 		wing.mtx.unlock();
// 	}, { task3 });

// 	bool done = true;//wing.wait(std::chrono::milliseconds(100));
// 	wing.join();
// 	REQUIRE(specialist1->speciality == ST_CPU);
// 	REQUIRE(specialist2->speciality == ST_CPU);
// 	REQUIRE(specialist3->speciality == ST_PCI);
// 	REQUIRE(specialist4->speciality == ST_HDD);
// 	REQUIRE(done == true);
// 	REQUIRE(k == 9);
// }

// TEST_CASE("Test for wait time") {
// 	atomic_int k = 5;
// 	const size_t COUNT = 6;
// 	Wing wing {};

// 	for (size_t i = 0; i < COUNT; i++) {
// 		auto task = wing.task(ST_CPU, [&]{
// 			std::this_thread::sleep_for(std::chrono::milliseconds(100));
// 			k++;
// 		});
// 	}
	
// 	wing.join();
// 	// REQUIRE(k == 5 + COUNT);
// 	// REQUIRE(static_cast<uint64_t>(wing.initTime() * 1e6) < 0);
// 	REQUIRE(static_cast<uint64_t>(wing.workTime() * 1e6) / static_cast<double>(1000) < 150);
// }

// TEST_CASE("Test for work after join") {
// 	atomic_int k = 5;
// 	const size_t COUNT = 6;
// 	Wing wing {};

// 	for (size_t i = 0; i < COUNT; i++) {
// 		auto task = wing.task(ST_CPU, [&]{
// 			std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 			k++;
// 		});
// 	}
	
// 	wing.join();
// 	REQUIRE(k == 5 + COUNT);

// 	for (size_t i = 0; i < COUNT; i++) {
// 		auto task = wing.task(ST_CPU, [&]{
// 			std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 			k++;
// 		});
// 	}

// 	wing.join();
// 	REQUIRE(k == 5 + 2 * COUNT);
// }

// TEST_CASE("Test idle task after regular tasks") {
// 	atomic_int k = 5;
// 	int m = 100;
// 	const size_t COUNT = 6;
// 	mutex mtx;
// 	Wing wing {};

// 	for (size_t i = 0; i < COUNT; i++) {
// 		auto task = wing.task(ST_CPU, [&]{
// 			std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 			k++;
// 		});
// 	}

// 	auto id = wing.idleTask(ST_CPU, [&]{
// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 		mtx.lock();
// 			m = 42;
// 		mtx.unlock();
// 	});

// 	wing.join();
// 	REQUIRE(k == 5 + COUNT);

// 	std::this_thread::sleep_for(std::chrono::milliseconds(20));
// 	REQUIRE(m == 42);
// }

// TEST_CASE("Test stopping idle task") {
// 	int m = 100;
// 	mutex mtx;
// 	Wing wing {};

// 	auto id = wing.idleTask(ST_CPU, [&]{
// 		std::this_thread::sleep_for(std::chrono::milliseconds(5));
// 		mtx.lock();
// 			m = 42;
// 		mtx.unlock();
// 	});

// 	std::this_thread::sleep_for(std::chrono::milliseconds(25));
// 	REQUIRE(m == 42);

// 	m = 100;
// 	std::this_thread::sleep_for(std::chrono::milliseconds(25));
// 	REQUIRE(m == 42);

// 	wing.stopIdleTask(id);
// 	wing.finish();
// 	m = 100;
// 	std::this_thread::sleep_for(std::chrono::milliseconds(25));
// 	REQUIRE(m == 100);
// }

// TEST_CASE("Test idle task to stop after wing destroy") {
// 	int m = 100;
// 	mutex mtx;

// 	{
// 		Wing wing {};

// 		auto id = wing.idleTask(ST_CPU, [&]{
// 			std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 			mtx.lock();
// 				m = 42;
// 			mtx.unlock();
// 		});

// 		std::this_thread::sleep_for(std::chrono::milliseconds(20));
// 		REQUIRE(m == 42);

// 		m = 100;
// 		std::this_thread::sleep_for(std::chrono::milliseconds(20));
// 		REQUIRE(m == 42);
// 	}

// 	std::this_thread::sleep_for(std::chrono::milliseconds(20));
// 	m = 100;
// 	std::this_thread::sleep_for(std::chrono::milliseconds(20));
// 	REQUIRE(m == 100);
// }

#endif