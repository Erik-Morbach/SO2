#pragma once
#include <cstddef>
#include <cstdio>
#include <bitset>
#include <mutex>

template<size_t TOTAL_MEM, size_t FRAME_SIZE>
class MainMemory {
private:
	static constexpr size_t QNT_FRAMES = TOTAL_MEM/FRAME_SIZE;
	std::bitset<QNT_FRAMES> used;
	char mem[QNT_FRAMES][FRAME_SIZE];
	size_t allocSize[QNT_FRAMES][FRAME_SIZE];
	mutable size_t pageFaults = 0;
	mutable std::mutex mtx;
public:
	size_t allocate(size_t size);
	void writeInto(char *src, size_t frame, size_t off, size_t n);
	void readInto(char *dst, size_t frame, size_t off, size_t n) const;
	void free(size_t pAddr);

	void printSummary() const;

	size_t getQntFrames() const { return QNT_FRAMES; }
	size_t getAllocSize(size_t pAddr) const { std::lock_guard<std::mutex> lock(mtx); return allocSize[pAddr][0]; }
	size_t getPageFaults() const { std::lock_guard<std::mutex> lock(mtx); return pageFaults; }
	void addPageFault() const { std::lock_guard<std::mutex> lock(mtx); pageFaults++; }
	void clearPageFaults() { std::lock_guard<std::mutex> lock(mtx); pageFaults = 0; }

};

template<size_t TOTAL_MEM, size_t FRAME_SIZE>
void MainMemory<TOTAL_MEM, FRAME_SIZE>::printSummary() const {
	std::lock_guard<std::mutex> lock(mtx);
	printf("=== MainMemory Summary ===\n");
	printf("  TOTAL_MEM: %zu | FRAME_SIZE: %zu | QNT_FRAMES: %zu\n", TOTAL_MEM, FRAME_SIZE, QNT_FRAMES);
	printf("  Used frames: %zu / %zu (%.1f%%)\n", used.count(), QNT_FRAMES, 100.0 * used.count() / QNT_FRAMES);
	printf("  Page faults: %zu\n", pageFaults);
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE>
size_t MainMemory<TOTAL_MEM, FRAME_SIZE>::allocate(size_t size) {
	std::lock_guard<std::mutex> lock(mtx);
	size_t qntFrames = (size + FRAME_SIZE - 1) / FRAME_SIZE;
	size_t choosen = -1;
	int currentQnt = 0;
	for (size_t i = 0; i < QNT_FRAMES; i++) {
		if (used[i]) {
			currentQnt = 0;
			continue;
		}
		currentQnt++;
		if (currentQnt >= (int)qntFrames) {
			choosen = i - currentQnt + 1;
			break;
		}
	}
	if (choosen == (size_t)-1) {
		pageFaults++;
		return -1;
	}
	allocSize[choosen][0] = size;
	for (size_t i = choosen; i < choosen + qntFrames; i++) {
		used[i] = 1;
	}
	return choosen;
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE>
void MainMemory<TOTAL_MEM, FRAME_SIZE>::writeInto(char *src, size_t pAddr, size_t off, size_t n) {
	std::lock_guard<std::mutex> lock(mtx);
	size_t frame = pAddr;
	size_t pos = off;
	size_t index = 0;
	while (index < n) {
		mem[frame][pos] = src[index];
		pos++;
		frame += pos / FRAME_SIZE;
		pos %= FRAME_SIZE;
		index++;
	}
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE>
void MainMemory<TOTAL_MEM, FRAME_SIZE>::readInto(char *dst, size_t pAddr, size_t off, size_t n) const {
	std::lock_guard<std::mutex> lock(mtx);
	size_t frame = pAddr;
	size_t pos = off;
	size_t index = 0;
	while (index < n) {
		dst[index] = mem[frame][pos];
		pos++;
		frame += pos / FRAME_SIZE;
		pos %= FRAME_SIZE;
		index++;
	}
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE>
void MainMemory<TOTAL_MEM, FRAME_SIZE>::free(size_t pAddr) {
	std::lock_guard<std::mutex> lock(mtx);
	size_t size = allocSize[pAddr][0];
	size_t qntFrames = (size + FRAME_SIZE - 1) / FRAME_SIZE;
	for (size_t i = pAddr; i < pAddr + qntFrames; i++) {
		used[i] = 0;
	}
}