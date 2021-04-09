#ifndef tests_here

#include "core/Ash.h"
#include "core/Crater.h"
#include "core/Lava.h"
#include "core/Mountain.h"
#include "core/Rocks.h"
#include "core/Tectonic.h"
#include "engine/Batcher.h"
#include "scene/Scene.h"
#include "team/Team.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

bool run(size_t runId) {
	auto startLoading = chrono::high_resolution_clock::now();

	Ash ash {};

	Team team {};
	team.initGpuSpecialists(nullptr);
	Batcher batcher { ash, team };

	// thread loadSceneThread([&](){
		// batcher.loadFolderNth("_crops_harvester", 12);
		// batcher.loadFolder("_crops_harvester");

		// batcher.loadFolderTeam("_crops_harvester");
		// batcher.loadFolderTeam("_crops_mess");
	// });

	batcher.loadFolderTeam("_crops_megafactory");
	// batcher.loadFolderTeam("_crops_uptoride");
	team.join();

	Mountain mountain { ash };
	Rocks rocks { ash,  mountain };
	Crater crater { ash, mountain, rocks };
	Lava lava { ash, mountain, rocks, crater };
	Tectonic tectonic { ash, mountain, rocks, crater, lava };

	// loadSceneThread.join();
	batcher.establish(mountain, rocks, crater, lava);
	team.join();

	Scene scene { batcher };
	scene.init();

	mountain.showWindow();

	auto finishLoading = chrono::high_resolution_clock::now();
	printf("Initialized in %.3fs\n", chrono::duration_cast<chrono::duration<double>>(finishLoading - startLoading).count());
	uint64_t frame = 0;
	chrono::time_point<chrono::high_resolution_clock> lastWindowTitleUpdate;

	double t = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - finishLoading).count();

	while (frame < 50 || true) {
		if (glfwWindowShouldClose(mountain.window)) return true;

		double new_t = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - finishLoading).count();
		double dt = new_t - t;
		t = new_t;
		scene.update(t, dt);
		batcher.update(t, dt);
		tectonic.drawFrame();
		glfwPollEvents();

		frame++;
		auto now = chrono::high_resolution_clock::now();
		auto afterWindowUpdate = chrono::duration_cast<chrono::duration<double>>(now - lastWindowTitleUpdate).count();
		if (afterWindowUpdate > 0.1) {
			lastWindowTitleUpdate = now;
			auto runtime = chrono::duration_cast<chrono::duration<double>>(now - finishLoading).count();
			double fps = frame / runtime;
			char str[100];
			sprintf(str, "run %lld   size %d x %d   frame %lld   fps %.0f   time %.2f   sprites %lld", runId, crater.extent.width, crater.extent.height, frame, fps, t, scene.sprites());
			glfwSetWindowTitle(mountain.window, str);
		}
	}

	return false;
}

int main() {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	printf("===============================================================================\n");

	try {
		run(0);
		// for (size_t i = 1; i <= 750; i++) {
		// 	printf("            ------------------ starting run %lld ------------------            \n", i);
		// 	if (run(i)) break;
		// }
	} catch (const exception& e) {
		cout << "ERROR " << e.what() << endl;
		return EXIT_FAILURE;
	}

	return 0;
}
#endif