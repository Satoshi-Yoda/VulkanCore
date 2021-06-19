#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../batch/Batch.h"
#include "../core/Lava.h"
#include "../team/Team.h"

using std::map;
using std::mutex;
using std::set;
using std::string;
using std::unique_ptr;
using std::unordered_map;

class Batcher {
public:
	Batcher(Ash& ash, Team& team);
	~Batcher();

	void loadFolder(string folder);
	void establish(Mountain& mountain, Rocks& rocks, Crater& crater, Lava& lava);
	size_t addInstance(string name, Instance instance);
	void removeInstance(string name, size_t index);
	void updateInstance(string name, size_t index, Instance instance);
	void update(double t, double dt);

private:
	Ash& ash;
	Team& team;
	Lava* lava;

	const Instance VACUUM { { 1e16f, 1e16f } };

	map<string, unique_ptr<Batch>> caves;
	map<string, Batch*> cavesPtr;
	map<string, size_t> indexes;
	set<string> resizedNames;
	map<string, vector<size_t>> touchedIndexes;

	mutex putMutex;

	size_t texturesBytes;

	vector<Vertex> initQuad(uint32_t w, uint32_t h);
	// void addSampleInstance(string name);
};
