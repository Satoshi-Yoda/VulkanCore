#ifndef LOADER_H
#define LOADER_H

#include <string>

using namespace std;

void loadTexture(string filename, void* &data, int *width, int *height, int *channels);
void freeTexture(void* &data);

#endif
