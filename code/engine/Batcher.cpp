#include "Batcher.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <mutex>
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
	size_t fileSizes = 0;

	for (const auto& entry : filesystem::recursive_directory_iterator(folder)) {
		auto path = entry.path();
		if (path.extension().string() == ".png") {
			fileSizes += entry.file_size();
			files.push_back(path);
		}
	}

	uint32_t chunk = files.size() / workers;
	uint32_t rest  = files.size() - chunk * workers;
	vector<thread> threads;

	mutex putMutex;

	for (uint32_t w = 0; w < workers; w++) {
		threads.push_back(thread([=, &putMutex](){
			uint32_t start = chunk * w;
			uint32_t length = (w == workers - 1) ? (chunk + rest) : chunk;
			for (uint32_t i = start; i < start + length; i++) {
				string name = files[i].stem().string();
				// string name = files[i].string();
				void* tempPixels;
				int tempWidth, tempHeight;

				// printf("Loader worker %d: loaded %s\n", w, filename.data());
				loadTexture(files[i].string(), tempPixels, &tempWidth, &tempHeight);

				putMutex.lock();
					pixels[name] = tempPixels;
					width[name]  = tempWidth;
					height[name] = tempHeight;
					indexes[name] = i;
					initQuad(name, width[name], height[name]);
					// addSampleInstance(name);
					// addSampleInstance(name);

					BatchCreateData data {};
					data.pixels = tempPixels;
					data.width  = tempWidth;
					data.height = tempHeight;
					data.vertices = vertices[name];
					data.instances = instances[name];
					batches[name] = data;
				putMutex.unlock();
			}
		}));
	}

	for (auto& t : threads) t.join();

	texturesBytes = 0;
	for (const auto& file : files) {
		string name = file.stem().string();
		// string name = file.string();
		texturesBytes += width[name] * height[name] * 4;
	}

	printf("Loaded %lld .png files (%.2f Mb files) from %s in %.3fs\n", files.size(), 1.0 * fileSizes / (1 << 20), folder.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
	printf("(%lld Mb textures)\n", texturesBytes / (1 << 20));
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

void Batcher::addSampleInstance(string name) {
	random_device random {};
	int border = 200;
	uniform_int_distribution<int> x { - (1600 - border) / 2, (1600 - border) / 2 };
	uniform_int_distribution<int> y { -  (900 - border) / 2,  (900 - border) / 2 };

	instances[name].push_back({ { x(random), y(random) } });
}

void Batcher::establish(Lava& lava) {
	this->lava = &lava;

	auto start = chrono::high_resolution_clock::now();

	// for (auto& it : batches) {
	// 	lava.addBatch(it.second);
	// }

	vector<BatchCreateData> dataVector {};
	for (auto& it : batches) {
		dataVector.push_back(it.second);
	}
	lava.addBatches(dataVector);

	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
	printf("Established %lld lava objects (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", pixels.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
}

void Batcher::addInstance(string name, Instance instance) {
	instances[name].push_back(instance);
	lava->resizeInstanceBuffer(indexes[name], instances[name]);
}

void Batcher::updateInstance(string name, size_t index, Instance instance) {
	instances[name][index] = instance;
	namesForUpdate.push_back(name);
}

void Batcher::update(double t, double dt) {
	for (auto& name : namesForUpdate) {
		// TODO make Lava::updateInstanceBuffers()
		lava->updateInstanceBuffer(indexes[name], instances[name]);
	}

	namesForUpdate.clear();

	// auto start = chrono::high_resolution_clock::now();
	// bool resized = false;

	// vector<size_t> indexVector;
	// vector<vector<Instance>> instancesVector;

	// for (auto& it : batches) {
	// 	string name = it.first;
	// 	if (t > instances[name].size()) {
	// 		addSampleInstance(name);
	// 		// lava.resizeInstanceBuffer(indexes[name], instances[name]);
	// 		indexVector.push_back(indexes[name]);
	// 		instancesVector.push_back(instances[name]);
	// 		resized = true;
	// 	}
	// }

	// if (resized) {
	// 	lava->resizeInstanceBuffers(indexVector, instancesVector);
	// 	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
	// 	printf("resized %.2f, sprites %.0f in %.5f ms\n", t, round(batches.size() * t), 1000 * time);
	// }
}
