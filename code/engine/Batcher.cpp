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

	size_t i = 0;
	for (const auto& entry : filesystem::directory_iterator(folder)) {
		string name = entry.path().stem().string();

		void* tempPixels;
		int tempWidth, tempHeight;
		loadTexture(entry.path().string(), tempPixels, &tempWidth, &tempHeight);

		BatchCreateData data {};
		data.pixels = tempPixels;
		data.width  = tempWidth;
		data.height = tempHeight;
		data.vertices = initQuad(tempWidth, tempHeight);
		batches[name] = data;
		indexes[name] = i;
		i++;
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

				loadTexture(files[i].string(), tempPixels, &tempWidth, &tempHeight);

				putMutex.lock();
					BatchCreateData data {};
					data.pixels = tempPixels;
					data.width  = tempWidth;
					data.height = tempHeight;
					data.vertices = initQuad(tempWidth, tempHeight);
					batches[name] = data;
					indexes[name] = i;
				putMutex.unlock();
			}
		}));
	}

	for (auto& t : threads) t.join();

	texturesBytes = 0;
	for (const auto& file : files) {
		string name = file.stem().string();
		// string name = file.string();
		texturesBytes += batches[name].width * batches[name].height * 4;
	}

	printf("Loaded %lld .png files (%.2f Mb files) from %s in %.3fs\n", files.size(), 1.0 * fileSizes / (1 << 20), folder.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
	printf("(%lld Mb textures)\n", texturesBytes / (1 << 20));
}

vector<Vertex> Batcher::initQuad(uint32_t w, uint32_t h) {
	float scale = 1.0f;

	int x_min = 0 - w * scale / 2;
	int x_max = x_min + w * scale;
	int y_min = 0 - h * scale / 2;
	int y_max = y_min + h * scale;

	vector<Vertex> result;

	result.push_back({ { x_min, y_max }, { 0.0f, 1.0f } });
	result.push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	result.push_back({ { x_min, y_min }, { 0.0f, 0.0f } });

	result.push_back({ { x_max, y_max }, { 1.0f, 1.0f } });
	result.push_back({ { x_max, y_min }, { 1.0f, 0.0f } });
	result.push_back({ { x_min, y_min }, { 0.0f, 0.0f } });

	return result;
}

void Batcher::addSampleInstance(string name) {
	random_device random {};
	int border = 200;
	uniform_int_distribution<int> x { - (1600 - border) / 2, (1600 - border) / 2 };
	uniform_int_distribution<int> y { -  (900 - border) / 2,  (900 - border) / 2 };

	batches[name].instances.push_back({ { x(random), y(random) } });
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
	printf("Established %lld lava objects (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", batches.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
}

// TODO implement removeInstance(string name, size_t index)
// TODO addInstance should find free instance and return its index

void Batcher::addInstance(string name, Instance instance) {
	// TODO resize 1 -> 2 -> 4 -> 8 -> ...
	// TODO implement vacuum instance
	batches[name].instances.push_back(instance);
	lava->resizeInstanceBuffer(indexes[name], batches[name].instances);
}

void Batcher::updateInstance(string name, size_t index, Instance instance) {
	batches[name].instances[index] = instance;
	namesForUpdate.insert(name);
}

void Batcher::update(double t, double dt) {
	for (auto& name : namesForUpdate) {
		// TODO make Lava::updateInstanceBuffers()
		lava->updateInstanceBuffer(indexes[name], batches[name].instances);
	}

	namesForUpdate.clear();

	// auto start = chrono::high_resolution_clock::now();
	// bool resized = false;

	// vector<size_t> indexVector;
	// vector<vector<Instance>> instancesVector;

	// for (auto& it : batches) {
	// 	string name = it.first;
	// 	if (t > batches[name].size()) {
	// 		addSampleInstance(name);
	// 		// lava.resizeInstanceBuffer(indexes[name], batches[name].instances);
	// 		indexVector.push_back(indexes[name]);
	// 		instancesVector.push_back(batches[name].instances);
	// 		resized = true;
	// 	}
	// }

	// if (resized) {
	// 	lava->resizeInstanceBuffers(indexVector, instancesVector);
	// 	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
	// 	printf("resized %.2f, sprites %.0f in %.5f ms\n", t, round(batches.size() * t), 1000 * time);
	// }
}
