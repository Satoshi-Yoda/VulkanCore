#ifndef BATCHER_H
#define BATCHER_H

#include <string>
#include <unordered_map>
// #include <vector>

// #include "../core/Lava.h"
// #include "../core/Tectonic.h"

using std::string;
using std::unordered_map;

class Batcher {
public:
	Batcher();
	~Batcher();

	void loadFolder(string folder);
	void loadFolderNth(string folder, uint32_t workers);

private:
	unordered_map<string, int> width, height;
	unordered_map<string, void*> pixels;
};

#endif