#include "Batcher.h"

#include <filesystem>
#include <iostream>
// #include <iomanip>

// #include "../core/Lava.h"
// #include "../core/Tectonic.h"
#include "../utils/Loader.h"

using namespace std;

Batcher::Batcher() {}

Batcher::~Batcher() {}

void Batcher::loadFolder(string folder) {
	// loadTexture("pictures/tile.png", pixels, &width, &height);

	// filesystem::directory_iterator end;

	// for (filesystem::directory_iterator it("./"); it != end; ++it) {
		// std::cout << *it << std::endl;                                    
	// }

	const string stringPath { "pictures" };
	const filesystem::path folderPath { stringPath };
	for (const auto& dirEntry : std::filesystem::directory_iterator(folderPath))
		cout << dirEntry.path().filename().string() << endl;
}
