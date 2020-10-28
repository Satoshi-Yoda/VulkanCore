#include "Batcher.h"

#include <filesystem>
#include <iostream>
#include <thread>

// #include "../core/Lava.h"
// #include "../core/Tectonic.h"
#include "../utils/Loader.h"

using namespace std;

Batcher::Batcher() {}

Batcher::~Batcher() {}

void Batcher::loadFolder(string folder) {
	auto start = chrono::high_resolution_clock::now();

	for (const auto& dirEntry : filesystem::directory_iterator(folder)) {
		string filename = dirEntry.path().filename().string();
		loadTexture(folder + "/" + filename, pixels[filename], &width[filename], &height[filename]);
	}

	printf("Loaded %s/*.png in %.3fs\n", folder.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
}

void Batcher::loadFolderNth(string folder, uint32_t workers) {
	auto start = chrono::high_resolution_clock::now();

	vector<string> filenames;

	for (const auto& dirEntry : filesystem::directory_iterator(folder)) {
		filenames.push_back(dirEntry.path().filename().string());
	}

	uint32_t chunk = filenames.size() / workers;
	uint32_t rest  = filenames.size() - chunk * workers;

	vector<thread> threads;

	for (uint32_t w = 0; w < workers; w++) {
		threads.push_back(thread([=](){
			uint32_t start = chunk * w;
			uint32_t length = (w == workers - 1) ? (chunk + rest) : chunk;
			for (uint32_t i = start; i < start + length; i++) {
				string filename = filenames[i];
				// printf("Loader worker %d: loaded %s\n", w, filename.data());
				loadTexture(folder + "/" + filename, pixels[filename], &width[filename], &height[filename]);
			}
		}));
	}

	for (auto& t : threads) {
		t.join();
	}

	printf("Loaded %d .png files from %s in %.3fs\n", filenames.size(), folder.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
}
