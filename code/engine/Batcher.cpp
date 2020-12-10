#include "Batcher.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "../utils/Loader.h"

using namespace std;

Batcher::Batcher() {}

Batcher::~Batcher() {}

void Batcher::loadFolder(string folder) {
	auto start = chrono::high_resolution_clock::now();

	size_t i = 0;
	for (const auto& entry : filesystem::directory_iterator(folder)) {
		string name = entry.path().stem().string();

		void* pixels;
		int width, height;
		loadTexture(entry.path().string(), pixels, &width, &height);

		vector<Vertex> vertices = initQuad(width, height);

		// Cave cave {};
		// cave.setWorkingData(vertices, width, height, pixels);

		unique_ptr<Cave> cave = make_unique<Cave>();
		cave->setName(name);
		cave->setWorkingData(vertices, width, height, pixels);

		// BatchCreateData data {};
		// data.pixels = pixels;
		// data.width  = width;
		// data.height = height;
		// data.vertices = vertices;
		// batches[name] = data;
		caves[name] = move(cave);
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
				void* pixels;
				int width, height;

				loadTexture(files[i].string(), pixels, &width, &height);

				vector<Vertex> vertices = initQuad(width, height);

				unique_ptr<Cave> cave = make_unique<Cave>();
				cave->setName(name);
				cave->setWorkingData(vertices, width, height, pixels);

				// Cave cave {};
				// cave.setName(name);
				// cave.setWorkingData(vertices, width, height, pixels);

				putMutex.lock();
					// BatchCreateData data {};
					// data.pixels = pixels;
					// data.width  = width;
					// data.height = height;
					// data.vertices = vertices;
					// batches[name] = data;
					caves[name] = move(cave);
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
		texturesBytes += caves[name]->width * caves[name]->height * 4;
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

	addInstance(name, { { x(random), y(random) } });
}

void Batcher::establish(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
	this->ash = &ash;
	this->lava = &lava;

	auto start = chrono::high_resolution_clock::now();

	// vector<BatchCreateData> dataVector {};
	// for (auto& it : batches) {
	// 	dataVector.push_back(it.second);
	// }
	// lava.addBatches(dataVector);

	for (auto& it : caves) {
		auto& cave = it.second;
		cave->setVulkanEntities(ash, mountain, rocks, crater);
		cave->establish(CaveAspects::STAGING_VERTICES | CaveAspects::STAGING_INSTANCES | CaveAspects::STAGING_TEXTURE);
		cave->establish(CaveAspects::LIVE_VERTICES | CaveAspects::LIVE_INSTANCES | CaveAspects::LIVE_TEXTURE); // TODO use sheduler worker with own commandBuffer as worker for this task
		cavesPtr[it.first] = it.second.get();
		lava.addCave(move(cave));
	}

	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
	printf("Established %lld caves (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", caves.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
}

size_t Batcher::addInstance(string name, Instance instance) {
	namesForUpdate.insert(name);

	size_t result = 0;

	// if (batches[name].free.size() == 0) {
	// 	size_t oldSize = batches[name].instances.size();
	// 	size_t newSize = (oldSize == 0) ? 1 : oldSize * 2;

	// 	batches[name].free.reserve(newSize);
	// 	batches[name].instances.reserve(newSize);
	// 	batches[name].instances.push_back(instance);

	// 	for (size_t i = oldSize + 1; i < newSize; i++) {
	// 		batches[name].instances.push_back(VACUUM);
	// 		batches[name].free.push_back(i);
	// 	}

	// 	lava->resizeInstanceBuffer(indexes[name], batches[name].instances);
	// 	printf("Resized batch '%s' for %lld instances\n", name.data(), batches[name].instances.size());
	// 	result = oldSize;

	// } else {
	// 	size_t freeSlot = batches[name].free.back();
	// 	batches[name].free.pop_back();
	// 	batches[name].instances[freeSlot] = instance;
	// 	result = freeSlot;
	// }

	if (cavesPtr[name]->vacuum.size() == 0) {
		size_t oldSize = cavesPtr[name]->instances.size();
		size_t newSize = (oldSize == 0) ? 1 : oldSize * 2;

		cavesPtr[name]->vacuum.reserve(newSize);
		cavesPtr[name]->instances.reserve(newSize);
		cavesPtr[name]->instances.push_back(instance);

		for (size_t i = oldSize + 1; i < newSize; i++) {
			cavesPtr[name]->instances.push_back(VACUUM);
			cavesPtr[name]->vacuum.push_back(i);
		}

		printf("Cave '%s' marked for resize for %lld instances\n", name.data(), cavesPtr[name]->instances.size());

		// assert(result == oldSize);
		result = oldSize;

	} else {
		size_t freeSlot = cavesPtr[name]->vacuum.back();
		cavesPtr[name]->vacuum.pop_back();
		cavesPtr[name]->instances[freeSlot] = instance;
		// printf("result %d == freeSlot %d\n", result, freeSlot);
		// assert(result == freeSlot);
		result = freeSlot;
	}

	return result;
}

void Batcher::removeInstance(string name, size_t index) {
	#ifdef use_validation
	char msg[100];
	sprintf(msg, "Batcher::removeInstance(%s, %lld) => No such index!\n", name.data(), index);
	(index < cavesPtr[name]->instances.size()) >> (*ash)(msg);
	#endif

	namesForUpdate.insert(name);

	// batches[name].instances[index] = VACUUM;
	// batches[name].free.push_back(index);

	cavesPtr[name]->instances[index] = VACUUM;
	cavesPtr[name]->vacuum.push_back(index);

	// TODO implement shrink, maybe (nope)
}

void Batcher::updateInstance(string name, size_t index, Instance instance) {
	#ifdef use_validation
	char msg[100];
	sprintf(msg, "Batcher::updateInstance(%s, %lld) => No such index!\n", name.data(), index);
	(index < cavesPtr[name]->instances.size()) >> (*ash)(msg);
	#endif

	// batches[name].instances[index] = instance;

	cavesPtr[name]->instances[index] = instance;

	namesForUpdate.insert(name);
}

void Batcher::update(double t, double dt) {
	for (auto& name : namesForUpdate) {
		// TODO make Lava::updateInstanceBuffers()
		// lava->updateInstanceBuffer(indexes[name], batches[name].instances);
		cavesPtr[name]->refresh(CaveAspects::STAGING_INSTANCES | CaveAspects::LIVE_INSTANCES);
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
