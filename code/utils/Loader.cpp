#include <string>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Loader.h"

using namespace std;

void loadTexture(string filename, void* &data, int *width, int *height, int *channels) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename.data(), width, height, channels, STBI_rgb_alpha);

	if (!pixels) {
		throw runtime_error("Failed to load texture image!");
	}
	data = reinterpret_cast<void*>(pixels);
}

void freeTexture(void* &data) {
	stbi_image_free(reinterpret_cast<stbi_uc*>(data));
}
