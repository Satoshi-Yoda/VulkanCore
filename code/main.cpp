#include "core/Ash.h"
#include "core/Crater.h"
#include "core/Lava.h"
#include "core/Mountain.h"
#include "core/Rocks.h"
#include "core/Tectonic.h"
#include "engine/Batcher.h"
#include "scene/Scene.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

int main() {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	printf("\n");

	try {
		auto startLoading = chrono::high_resolution_clock::now();

		Batcher batcher {};
		// Scene scene {};

		thread loadSceneThread([&](){
			// scene.load();
			batcher.loadFolderNth("_crops_uptoride", 12);
		});

		Ash ash {};
		Mountain mountain { ash };
		Rocks rocks { ash,  mountain };
		Crater crater { ash, mountain, rocks };
		Lava lava { ash, mountain, rocks, crater };
		Tectonic tectonic { ash, mountain, rocks, crater, lava };

		loadSceneThread.join();
		// scene.establish(lava);
		batcher.establish(lava);

		mountain.showWindow();

		auto finishLoading = chrono::high_resolution_clock::now();
		printf("Initialized in %.3fs\n", chrono::duration_cast<chrono::duration<double>>(finishLoading - startLoading).count());
		uint64_t frame = 0;
		chrono::time_point<chrono::high_resolution_clock> lastWindowTitleUpdate;

		double t = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - finishLoading).count();

		while (!glfwWindowShouldClose(mountain.window)) {
			double new_t = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - finishLoading).count();
			double dt = new_t - t;
			t = new_t;
			// scene.update(lava, t, dt);
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
				sprintf(str, "size %d x %d   frame %lld   fps %.0f   time %.2f", crater.extent.width, crater.extent.height, frame, fps, t);
				glfwSetWindowTitle(mountain.window, str);
			}
		}

	} catch (const exception& e) {
		cout << "ERROR" << e.what() << endl;
		return EXIT_FAILURE;
	}

	return 0;
}
