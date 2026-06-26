#pragma once
#include <cstddef>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <bit>

#include "pageTable.hpp"
#include "mainMemory.hpp"
#include "tlb.hpp"

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
class MMU {
private:
	std::shared_ptr<MainMemory<TOTAL_MEM, FRAME_SIZE>> memory;
	std::unordered_map<std::thread::id, PageTable<TOTAL_VMEM, FRAME_SIZE>> pageTables;
	TLB<TLB_ENTRIES> tlb;
	mutable std::mutex mtx;

	std::pair<size_t, size_t> split(size_t vAddr);
	std::pair<size_t, size_t> transform(std::pair<size_t, size_t> pageAndOff);
public:
	MMU(std::shared_ptr<MainMemory<TOTAL_MEM, FRAME_SIZE>> mem);

	int allocate(size_t vAddr, size_t size);
	int read(void *dst, size_t vAddr, size_t size);
	int write(void *src, size_t vAddr, size_t size);
	int free(size_t vAddr);

	void printSummary() const;

	size_t getPageFaults() const { return memory->getPageFaults(); }
	void clearPageFaults() { memory->clearPageFaults(); }
};

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>::MMU(
	std::shared_ptr<MainMemory<TOTAL_MEM, FRAME_SIZE>> mem)
	: memory(mem) {}

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
std::pair<size_t, size_t> MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>::split(size_t vAddr) {
	size_t pageNumber = vAddr / FRAME_SIZE;
	size_t offset = vAddr % FRAME_SIZE;
	return {pageNumber, offset};
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
std::pair<size_t, size_t> MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>::transform(
	std::pair<size_t, size_t> pageAndOff) {
	size_t pageNumber = pageAndOff.first;
	size_t offset = pageAndOff.second;
	std::thread::id this_id = std::this_thread::get_id();

	if (tlb.exist(this_id, pageNumber)) {
		PageEntry entry = tlb.get(this_id, pageNumber);
		return {entry.getPAddr(), offset};
	}

	PageTable<TOTAL_VMEM, FRAME_SIZE> &pt = pageTables[this_id];
	PageEntry entry = pt.getEntry(pageNumber * FRAME_SIZE);

	if (entry.getVAddr() == (size_t)-1) {
		memory->addPageFault();
		return {(size_t)-1, 0};
	}

	tlb.addPageEntry(this_id, PageEntry(pageNumber, entry.getPAddr()));
	return {entry.getPAddr(), offset};
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
int MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>::allocate(size_t vAddr, size_t size) {
	std::lock_guard<std::mutex> lock(mtx);
	size_t pAddr = memory->allocate(size);
	if (pAddr == (size_t)-1) {
		return -1;
	}

	std::thread::id this_id = std::this_thread::get_id();
	PageTable<TOTAL_VMEM, FRAME_SIZE> &pt = pageTables[this_id];
	size_t numPages = (size + FRAME_SIZE - 1) / FRAME_SIZE;

	size_t baseVAddr = vAddr - (vAddr % FRAME_SIZE);
	for (size_t i = 0; i < numPages; i++) {
		const auto pageVAddr = baseVAddr + i * FRAME_SIZE;
		const auto pagePAddr = pAddr + i;
		pt.createNew(pageVAddr, pagePAddr);
		tlb.addPageEntry(this_id, PageEntry(pageVAddr / FRAME_SIZE, pagePAddr));
	}

	return 0;
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
int MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>::read(void *dst, size_t vAddr, size_t size) {
	std::lock_guard<std::mutex> lock(mtx);
	auto [pageNumber, offset] = split(vAddr);
	auto [pAddr, off] = transform({pageNumber, offset});

	if (pAddr == (size_t)-1) {
		return -1;
	}

	memory->readInto((char*)dst, pAddr, off, size);
	return 0;
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
int MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>::write(void *src, size_t vAddr, size_t size) {
	std::lock_guard<std::mutex> lock(mtx);
	auto [pageNumber, offset] = split(vAddr);
	auto [pAddr, off] = transform({pageNumber, offset});

	if (pAddr == (size_t)-1) {
		return -1;
	}

	memory->writeInto((char*)src, pAddr, off, size);
	return 0;
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
void MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>::printSummary() const {
	std::lock_guard<std::mutex> lock(mtx);
	printf("=== MMU Summary ===\n");
	printf("  TOTAL_MEM=%zu FRAME_SIZE=%zu TOTAL_VMEM=%zu TLB_ENTRIES=%zu\n",
		TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES);
	printf("  Active page tables: %zu\n", pageTables.size());
	for (const auto &[tid, _] : pageTables) {
		printf("    thread=%zx\n", std::hash<std::thread::id>()(tid));
	}
	tlb.printSummary();
	memory->printSummary();
}

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
int MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>::free(size_t vAddr) {
	std::lock_guard<std::mutex> lock(mtx);
	std::thread::id this_id = std::this_thread::get_id();
	PageTable<TOTAL_VMEM, FRAME_SIZE> &pt = pageTables[this_id];
	size_t baseVAddr = (vAddr / FRAME_SIZE) * FRAME_SIZE;

	PageEntry firstEntry = pt.getEntry(baseVAddr);
	if (firstEntry.getVAddr() == (size_t)-1) {
		return -1;
	}

	size_t pAddr = firstEntry.getPAddr();
	size_t allocSize = memory->getAllocSize(pAddr);
	size_t numPages = (allocSize + FRAME_SIZE - 1) / FRAME_SIZE;

	memory->free(pAddr);
	for (size_t i = 0; i < numPages; i++) {
		size_t pageVAddr = baseVAddr + i * FRAME_SIZE;
		pt.removeEntry(pageVAddr);
		tlb.remove(this_id, pageVAddr / FRAME_SIZE);
	}

	return 0;
}