#pragma once
#include <cstddef>

template<size_t TOTAL_MEM, size_t FRAME_SIZE>
class MainMemory {
private:
	char mem[TOTAL_MEM/FRAME_SIZE][FRAME_SIZE];
	size_t allocSize[TOTAL_MEM/FRAME_SIZE][FRAME_SIZE];
public:
	size_t allocate(size_t size);
	void writeInto(void *src, size_t pAddr, size_t off, size_t n);
	void readInto(void *dst, size_t pAddr, size_t off, size_t n);
	void free(size_t size);
};