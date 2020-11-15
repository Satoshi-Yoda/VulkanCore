#ifndef BATCHER_H
#define BATCHER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "../core/Lava.h"

using std::string;
using std::unordered_map;

class Batcher {
public:
	Batcher();
	~Batcher();

	void loadFolder(string folder);
	void loadFolderNth(string folder, uint32_t workers = 1);
	void establish(Lava& lava);
	void addInstance(string name, Instance instance);

private:
	unordered_map<string, int> width, height;
	unordered_map<string, void*> pixels;
	unordered_map<string, vector<Vertex>> vertices;
	unordered_map<string, vector<Instance>> instances;

	unordered_map<string, BatchCreateData> batches;

	size_t texturesBytes;

	void initQuad(string name, uint32_t w, uint32_t h);
	void initSampleInstance(string name);
};

#endif