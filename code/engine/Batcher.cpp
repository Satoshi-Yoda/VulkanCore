#include "Batcher.h"

#include <filesystem>
#include <iostream>
#include <random>
#include <thread>

// #include "../core/Lava.h"
// #include "../core/Tectonic.h"
#include "../utils/Loader.h"

using namespace std;

Batcher::Batcher() {}

Batcher::~Batcher() {}

void Batcher::loadFolder(string folder) {
	auto start = chrono::high_resolution_clock::now();

	for (const auto& entry : filesystem::directory_iterator(folder)) {
		string name = entry.path().stem().string();
		loadTexture(entry.path().string(), pixels[name], &width[name], &height[name]);
	}

	printf("Loaded %s/*.png in %.3fs\n", folder.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
}

void Batcher::loadFolderNth(string folder, uint32_t workers) {
	auto start = chrono::high_resolution_clock::now();

	vector<filesystem::path> files;

	for (const auto& entry : filesystem::directory_iterator(folder)) {
		// filenames.push_back(entry.path().filename().string());
		files.push_back(entry.path());
	}

	uint32_t chunk = files.size() / workers;
	uint32_t rest  = files.size() - chunk * workers;

	vector<thread> threads;

	for (uint32_t w = 0; w < workers; w++) {
		threads.push_back(thread([=](){
			uint32_t start = chunk * w;
			uint32_t length = (w == workers - 1) ? (chunk + rest) : chunk;
			for (uint32_t i = start; i < start + length; i++) {
				string name = files[i].stem().string();
				// printf("Loader worker %d: loaded %s\n", w, filename.data());
				loadTexture(files[i].string(), pixels[name], &width[name], &height[name]);
				initQuad(name, width[name], height[name]);
				initSampleInstance(name);
			}
		}));
	}

	for (auto& t : threads) {
		t.join();
	}

	printf("Loaded %d .png files from %s in %.3fs\n", files.size(), folder.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
}

void Batcher::initQuad(string name, uint32_t w, uint32_t h) {
	float scale = 1.0f;

	int x_min = 0 - w * scale / 2;
	int x_max = x_min + w * scale;
	int y_min = 0 - h * scale / 2;
	int y_max = y_min + h * scale;

	vertices[name].push_back({ { x_min, y_max }, { 0.0f, 1.0f } });
	vertices[name].push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	vertices[name].push_back({ { x_min, y_min }, { 0.0f, 0.0f } });

	vertices[name].push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	vertices[name].push_back({ { x_max, y_min }, { 1.0f, 0.0f } });
	vertices[name].push_back({ { x_min, y_min }, { 0.0f, 0.0f } });
}

void Batcher::initSampleInstance(string name) {
	random_device random {};
	int border = 200;
	uniform_int_distribution<int> x { - (1600 - border) / 2, (1600 - border) / 2 };
	uniform_int_distribution<int> y { -  (900 - border) / 2,  (900 - border) / 2 };

	instances[name].push_back({ { x(random), y(random) } });
}

void Batcher::establish(Lava& lava) {
	for (auto& it : pixels) {
		string key = it.first;
		auto lavaObjectId = lava.addObject(vertices[key], instances[key], width[key], height[key], pixels[key]);
	}
}

void Batcher::addInstance(string name, Instance instance) {

}
