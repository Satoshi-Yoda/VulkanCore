#include "Batcher.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include "../state/flag_group.hpp"
#include "../utils/Loader.h"

using namespace std;

Batcher::Batcher(Ash& ash, Team& team): ash(ash), team(team) {}

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

		unique_ptr<Cave> cave = make_unique<Cave>(ash);
		cave->setName(name);
		cave->setWorkingData(vertices, width, height, pixels);

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
		threads.push_back(thread([=, &putMutex, this](){
			uint32_t start = chunk * w;
			uint32_t length = (w == workers - 1) ? (chunk + rest) : chunk;
			for (uint32_t i = start; i < start + length; i++) {
				string name = files[i].stem().string();
				// string name = files[i].string();
				void* pixels;
				int width, height;

				loadTexture(files[i].string(), pixels, &width, &height);

				vector<Vertex> vertices = initQuad(width, height);

				unique_ptr<Cave> cave = make_unique<Cave>(ash);
				cave->setName(name);
				cave->setWorkingData(vertices, width, height, pixels);

				putMutex.lock();
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
}

void Batcher::loadFolderTeam(string folder) {
	auto start = chrono::high_resolution_clock::now();

	size_t i = 0;
	for (const auto& entry : filesystem::recursive_directory_iterator(folder)) {
		auto path = entry.path();
		if (path.extension().string() == ".png") {
			size_t index = i++;
			team.task(ST_CPU, [path, index, this]{
				string name = path.stem().string();

				void* pixels;
				int width, height;
				loadTexture(path.string(), pixels, &width, &height);

				vector<Vertex> vertices = initQuad(width, height);

				unique_ptr<Cave> cave = make_unique<Cave>(ash);
				cave->setName(name);
				cave->setWorkingData(vertices, width, height, pixels);

				putMutex.lock();
					caves[name] = move(cave);
					indexes[name] = index;
				putMutex.unlock();
			});
		}
	}

	// team.join();

	printf("Loaded %lld files: %s/*.png in %.3fs\n", i, folder.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
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

// void Batcher::addSampleInstance(string name) {
// 	random_device random {};
// 	int border = 200;
// 	uniform_int_distribution<int> x { - (1600 - border) / 2, (1600 - border) / 2 };
// 	uniform_int_distribution<int> y { -  (900 - border) / 2,  (900 - border) / 2 };

// 	addInstance(name, { { x(random), y(random) } });
// }

// void Batcher::establish(Ash& ash, Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
// 	this->ash = &ash;
// 	this->lava = &lava;

// 	auto start = chrono::high_resolution_clock::now();

// 	for (auto& it : caves) {
// 		auto& cave = it.second;
// 		cave->setVulkanEntities(ash, mountain, rocks, crater);
// 		cave->establish(CaveAspect::STAGING_VERTICES, CaveAspect::STAGING_INSTANCES, CaveAspect::STAGING_TEXTURE);
// 		cave->establish(CaveAspect::LIVE_VERTICES, CaveAspect::LIVE_INSTANCES, CaveAspect::LIVE_TEXTURE); // TODO use sheduler worker with own commandBuffer as worker for this task
// 		cave->free(CaveAspect::STAGING_VERTICES, CaveAspect::STAGING_TEXTURE); // TODO free working versices & texture also
// 		cavesPtr[it.first] = it.second.get();
// 		lava.addCave(move(cave));
// 	}

// 	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
// 	printf("Established %lld caves (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", caves.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
// }

void Batcher::establish(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
	this->lava = &lava;

	auto start = chrono::high_resolution_clock::now();

	// team.join();

	for (auto& it : caves) {\
		auto& key = it.first;
		auto& cave = it.second;
		auto id1 = team.task(ST_CPU, [&cave, &mountain, &rocks, &crater, &lava, this]{
			cave->setVulkanEntities(mountain, rocks, crater);
			cave->establish(CaveAspect::STAGING_VERTICES, CaveAspect::STAGING_INSTANCES, CaveAspect::STAGING_TEXTURE);
		});

		team.task(ST_GPU, [&key, &cave, &mountain, &rocks, &crater, &lava, this]{
			cave->establish(CaveAspect::LIVE_VERTICES, CaveAspect::LIVE_INSTANCES, CaveAspect::LIVE_TEXTURE); // TODO use sheduler worker with own commandBuffer as worker for this task
			cave->free(CaveAspect::STAGING_VERTICES, CaveAspect::STAGING_TEXTURE); // TODO free working versices & texture also
			cavesPtr[key] = cave.get();
			lava.addCave(move(cave));
		}, { id1 });
	}

	// team.join();

	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
	printf("Established %lld caves (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", caves.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
}

size_t Batcher::addInstance(string name, Instance instance) {
	size_t result = 0;

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

		resizedNames.insert(name);
		// printf("Cave '%s' marked for resize for %lld instances\n", name.data(), cavesPtr[name]->instances.size());
		result = oldSize;

	} else {
		size_t freeSlot = cavesPtr[name]->vacuum.back();
		cavesPtr[name]->vacuum.pop_back();
		cavesPtr[name]->instances[freeSlot] = instance;
		result = freeSlot;
	}

	touchedIndexes[name].push_back(result);

	return result;
}

void Batcher::removeInstance(string name, size_t index) {
	#ifdef use_validation
	char msg[100];
	sprintf(msg, "Batcher::removeInstance(%s, %lld) => No such index!\n", name.data(), index);
	(index < cavesPtr[name]->instances.size()) >> ash(msg);
	#endif

	cavesPtr[name]->instances[index] = VACUUM;
	cavesPtr[name]->vacuum.push_back(index);
	touchedIndexes[name].push_back(index);
}

void Batcher::updateInstance(string name, size_t index, Instance instance) {
	#ifdef use_validation
	char msg[100];
	sprintf(msg, "Batcher::updateInstance(%s, %lld) => No such index!\n", name.data(), index);
	(index < cavesPtr[name]->instances.size()) >> ash(msg);
	#endif

	cavesPtr[name]->instances[index] = instance;
	touchedIndexes[name].push_back(index);
}

void Batcher::update(double t, double dt) {
	for (auto& [name, value] : touchedIndexes) {
		if (resizedNames.find(name) != resizedNames.end() || value.size() * 100 > cavesPtr[name]->instanceCount) {
			cavesPtr[name]->refresh(CaveAspect::STAGING_INSTANCES, CaveAspect::LIVE_INSTANCES);
		} else {
			cavesPtr[name]->updateInstances(value);
		}
		value.clear();
	}

	resizedNames.clear();
}
