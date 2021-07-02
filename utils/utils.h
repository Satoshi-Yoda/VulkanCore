#pragma once

#include <thread>

using std::vector;
using std::thread;

void memcpy_parallel(void* dst, const void* src, size_t size, uint32_t workers) {
	unsigned char*       dstChar = reinterpret_cast<unsigned char*>(dst);
	const unsigned char* srcChar = reinterpret_cast<const unsigned char*>(src);

	size_t chunk = size / workers;
	size_t rest = size - chunk * workers;

	vector<thread> threads;

	for (uint32_t i = 0; i < workers; i++) {
		const unsigned char* srcChunk = srcChar + i * chunk;
		unsigned char*       dstChunk = dstChar + i * chunk;
		size_t sizeChunk = (i == workers - 1) ? chunk + rest : chunk;
		threads.push_back(thread([=](){
			memcpy(dstChunk, srcChunk, sizeChunk);
		}));
	}

	for (auto& t : threads) {
		t.join();
	}
}
