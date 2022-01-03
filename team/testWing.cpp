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

TEST_CASE("Wing: Test waiting for few tasks") {
	atomic_int k = 5;
	Wing wing {};

	wing.task([&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	wing.task([&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	wing.task([&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	wing.join();
	REQUIRE(k == 8);
}

TEST_CASE("Wing: Test waiting for a lot of tasks") {
	atomic_int k = 5;
	const size_t COUNT = 100;
	Wing wing {};

	for (size_t i = 0; i < COUNT; i++) {
		wing.task( [&]{
			k++;
		});
	}
	wing.join();
	REQUIRE(k == 5 + COUNT);
}

TEST_CASE("Wing: Test for managing task id") {
	atomic_int k = 5;
	Wing wing {};
	auto id1 = wing.task( [&]{ k = 10; });
	auto id2 = wing.task( [&]{ k = 10; });
	wing.join();
	REQUIRE(id1 != id2);
}

TEST_CASE("Wing: Test for join to complete all tasks") {
	atomic_int k = 0;
	Wing wing {};

	for (int i = 0; i < 100; i++) {
		wing.task([&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			k++;
		});
	}

	wing.join();
	REQUIRE(k == 100);
}

TEST_CASE("Wing: Test for finding assigned specialist") {
	Technician* specialist;
	Wing wing {};
	wing.task([&]{
		wing.mtx.lock();
			specialist = wing.findCurrentTechnician();
		wing.mtx.unlock();
	});

	wing.join();

	REQUIRE(specialist != nullptr);
	REQUIRE(specialist->id > 100);
}

TEST_CASE("Wing: threads count") {
	Wing wing1 {};
	REQUIRE(wing1.cpuThreads == 12);

	Wing wing2 { 4 };
	REQUIRE(wing2.cpuThreads == 4);
}

TEST_CASE("Wing: Test several specialists even distribution") {
	set<size_t> ids;
	Wing wing {};

	for (size_t i = 0; i < wing.cpuThreads; i++) {
		wing.task([&]{
			wing.mtx.lock();
				ids.insert(wing.findCurrentTechnician()->id);
			wing.mtx.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		});
	}

	wing.join();

	REQUIRE(ids.size() == wing.cpuThreads);

	for (size_t i = 0; i < wing.cpuThreads; i++) {
		if (ids.find(101 + i) == ids.end()) {
			for (auto id : ids) cout << id << ' ';
			cout << endl;
		}
		REQUIRE(ids.find(101 + i) != ids.end());
	}
}

TEST_CASE("Wing: Test dependency") {
	atomic_int k = 5;
	Wing wing {};

	auto firstTask = wing.task([&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	auto secondTask = wing.task([&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		k++;
	});
	auto thirdTask = wing.task([&]{
		k = (k == 7) ? 10 : 0;
	}, { firstTask, secondTask });

	wing.join();
	REQUIRE(k == 10);
}

TEST_CASE("Wing: Test for completed dependency") {
	atomic_int k = 5;
	Wing wing {};

	auto firstTask = wing.task([&]{
		k++;
	});
	auto secondTask = wing.task([&]{
		k++;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	auto thirdTask = wing.task([&]{
		k = (k == 7) ? 10 : 0;
	}, { firstTask, secondTask });

	bool done = true;//wing.wait(std::chrono::milliseconds(100));
	wing.join();
	REQUIRE(done == true);
	REQUIRE(k == 10);
}

TEST_CASE("Wing: Test for wait time") {
	atomic_int k = 5;
	const size_t COUNT = 6;
	Wing wing {};

	for (size_t i = 0; i < COUNT; i++) {
		auto task = wing.task([&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			k++;
		});
	}
	
	wing.join();
	REQUIRE(k == 5 + COUNT);
	REQUIRE(static_cast<uint64_t>(wing.workTime() * 1e6) / static_cast<double>(1000) < 150);
}

TEST_CASE("Wing: Test for work after join") {
	atomic_int k = 5;
	const size_t COUNT = 6;
	Wing wing {};

	for (size_t i = 0; i < COUNT; i++) {
		auto task = wing.task([&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			k++;
		});
	}
	
	wing.join();
	REQUIRE(k == 5 + COUNT);

	for (size_t i = 0; i < COUNT; i++) {
		auto task = wing.task([&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			k++;
		});
	}

	wing.join();
	REQUIRE(k == 5 + 2 * COUNT);
}

TEST_CASE("Wing: Test idle task after regular tasks") {
	atomic_int k = 5;
	int m = 100;
	const size_t COUNT = 6;
	mutex mtx;
	Wing wing {};

	for (size_t i = 0; i < COUNT; i++) {
		auto task = wing.task([&]{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			k++;
		});
	}

	auto id = wing.idleTask([&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		mtx.lock();
			m = 42;
		mtx.unlock();
	});

	wing.join();
	REQUIRE(k == 5 + COUNT);

	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	REQUIRE(m == 42);
}

TEST_CASE("Wing: Test stopping idle task") {
	int m = 100;
	mutex mtx;
	Wing wing {};

	auto id = wing.idleTask([&]{
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

	wing.stopIdleTask(id);
	wing.finish();
	m = 100;
	std::this_thread::sleep_for(std::chrono::milliseconds(25));
	REQUIRE(m == 100);
}

TEST_CASE("Wing: Test idle task to stop after wing destroy") {
	int m = 100;
	mutex mtx;

	{
		Wing wing {};

		auto id = wing.idleTask([&]{
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

#endif