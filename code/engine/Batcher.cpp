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



// void Batcher::establish(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
// 	this->lava = &lava;

// 	auto start = chrono::high_resolution_clock::now();

// 	VkCommandBuffer cb_u;

// 	auto id0 = team.gpuTask([&rocks, &cb_u, this](VkCommandBuffer cb){
// 		cb_u = rocks.beginSingleTimeCommands();
// 	});

// 	set<shared_ptr<Task>> deps;
// 	for (auto& it : caves) {
// 		auto& key = it.first;
// 		auto& cave = it.second;

// 		auto id1 = team.task(ST_CPU, [&cave, &mountain, &rocks, &crater, &lava, this]{
// 			cave->setVulkanEntities(mountain, rocks, crater);
// 			cave->establish(CaveAspect::STAGING_VERTICES, CaveAspect::STAGING_INSTANCES, CaveAspect::STAGING_TEXTURE);
// 		});

// 		auto id2 = team.gpuTask([&key, &cave, &mountain, &rocks, &crater, &lava, this](VkCommandBuffer cb){
// 			cave->establish(CaveAspect::LIVE_VERTICES, CaveAspect::LIVE_INSTANCES, CaveAspect::LIVE_TEXTURE);
// 		}, { id1 });
// 		deps.insert(id2);
// 	}

// 	team.gpuTask([&rocks, &cb_u, this](VkCommandBuffer cb){
// 		rocks.endSingleTimeCommands(cb_u);
// 	}, { id0 });

// 	auto id3 = team.gpuTask([&rocks, this](VkCommandBuffer cb){
// 	}, deps);

// 	auto id4 = team.gpuTask([&rocks, &lava, this](VkCommandBuffer cb){
// 		for (auto& it : caves) {
// 			auto& key = it.first;
// 			auto& cave = it.second;
// 			cavesPtr[key] = cave.get();
// 			lava.addCave(move(cave));
// 		}
// 	}, { id3 });

// 	auto time = chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count();
// 	printf("Established %lld caves (%lld Mb textures) in %.3fs (%.2f Gb/s)\n", caves.size(), texturesBytes / (1 << 20), time, texturesBytes / time / (1 << 30));
// }



void Batcher::establish(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava) {
	this->lava = &lava;

	auto start = chrono::high_resolution_clock::now();

	set<shared_ptr<Task>> deps;
	for (auto& it : caves) {
		auto& cave = it.second;
		cave->setVulkanEntities(mountain, rocks, crater);

		deps.insert(team.task(ST_CPU, [&cave, this]{
			cave->establish(CaveAspect::STAGING_VERTICES, CaveAspect::STAGING_INSTANCES, CaveAspect::STAGING_TEXTURE);
		}));
	}

	auto id = team.gpuTask([this](VkCommandBuffer cb){
		for (auto& it : caves) {
			auto& cave = it.second;
			cave->establish(cb, CaveAspect::LIVE_VERTICES, CaveAspect::LIVE_INSTANCES, CaveAspect::LIVE_TEXTURE);
		}
	}, deps);

	team.gpuTask([&lava, this](VkCommandBuffer cb){
		for (auto& it : caves) {
			auto& key = it.first;
			auto& cave = it.second;
			cave->free(CaveAspect::STAGING_VERTICES, CaveAspect::STAGING_TEXTURE); // TODO free working versices & texture also
			cavesPtr[key] = cave.get();
			lava.addCave(move(cave));
		}
	}, { id });

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
		} else if (value.size() > 0) {
			cavesPtr[name]->updateInstances(value);
		}
		value.clear();
	}

	resizedNames.clear();
}
