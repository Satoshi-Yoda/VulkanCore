#include "Loader.h"

#include <chrono>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;

void loadTexture(string filename, void* &data, int *width, int *height) {
	auto start = chrono::high_resolution_clock::now();

	int channels;
	stbi_uc* pixels = stbi_load(filename.data(), width, height, &channels, STBI_rgb_alpha);
	data = reinterpret_cast<void*>(pixels);

	printf("Loaded %s in %.3fs\n", filename.data(), chrono::duration_cast<chrono::duration<double>>(chrono::high_resolution_clock::now() - start).count());
}

void freeTexture(void* &data) {
	stbi_image_free(reinterpret_cast<stbi_uc*>(data));
}
