#pragma once
#include <cstddef>
#include <memory>
#include <unordered_map>

#include "pageTable.hpp"
#include "mainMemory.hpp"
#include "tlb.hpp"

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM>
class MMU {
private:
	std::shared_ptr<MainMemory<TOTAL_MEM, FRAME_SIZE>> memory; // Todo mundo compartilha a mesma memória base
	std::unordered_map<int, PageTable<TOTAL_VMEM, FRAME_SIZE>> pageTables; // Cada PID tem a sua pageTable
	PageTable<TOTAL_VMEM, FRAME_SIZE> &currentPage;
	TLB tlb;

	std::pair<size_t, size_t> split(size_t vAddr);
	std::pair<size_t, size_t> transform(std::pair<size_t, size_t> pageAndOff);
public:
	int allocate(size_t vAddr, size_t size);
	int read(void *dst, size_t vAddr, size_t size);
	int write(void *src, size_t vAddr, size_t size);
	int free(size_t vAddr, size_t size);
};