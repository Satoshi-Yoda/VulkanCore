#include "engine/Ash.h"
#include "engine/Crater.h"
#include "engine/Lava.h"
#include "engine/Mountain.h"
#include "engine/Rocks.h"
#include "engine/Tectonic.h"
#include "scene/Scene.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

int main() {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	printf("\n");

	try {
		auto start_createInstance = chrono::high_resolution_clock::now();

		Scene scene {};

		thread loadSceneThread([&](){
			scene.load();
		});

		Ash ash {};
		Mountain mountain { ash };
		Rocks rocks { ash,  mountain };
		Crater crater { ash, mountain, rocks };
		Lava lava { ash, mountain, rocks, crater };
		Tectonic tectonic { ash, mountain, rocks, crater, lava };

		loadSceneThread.join();
		scene.establish(lava);

		mountain.showWindow();

		auto finishLoading = chrono::high_resolution_clock::now();
		printf("Initialized in %.3fs\n", chrono::duration_cast<chrono::duration<double>>(finishLoading - start_createInstance).count());
		uint64_t frame = 0;
		chrono::time_point<chrono::high_resolution_clock> lastWindowTitleUpdate;

		double t = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - finishLoading).count();

		while (!glfwWindowShouldClose(mountain.window)) {
			double new_t = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - finishLoading).count();
			double dt = new_t - t;
			t = new_t;
			scene.update(lava, t, dt);
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
