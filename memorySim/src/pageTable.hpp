#pragma once
#include <algorithm>
#include <iostream>
#include <functional>
#include <cstddef>
#include <cstdio>
#include <unordered_set>

#include "pageEntry.hpp"

template<size_t TOTAL_VMEM, size_t PAGE_SIZE>
class PageTable {
private:
	std::unordered_set<PageEntry> entries;
public:
	PageEntry getEntry(size_t vAddr) const;
	PageEntry createNew(size_t vAddr, size_t pAddr);
	bool removeEntry(size_t vAddr);
	void printSummary() const;
};

template<size_t TOTAL_VMEM, size_t PAGE_SIZE>
PageEntry PageTable<TOTAL_VMEM, PAGE_SIZE>::getEntry(size_t vAddr) const{
	auto it = entries.find({vAddr, 0});
	if (it != entries.end()) {
		return *it;
	}
	return PageEntry(-1, -1);
}

template<size_t TOTAL_VMEM, size_t PAGE_SIZE>
bool PageTable<TOTAL_VMEM, PAGE_SIZE>::removeEntry(size_t vAddr) {
	auto it = entries.find({vAddr, 0});
	if (it == entries.end()) {
		return false;
	}
	entries.erase(it);
	return true;
}

template<size_t TOTAL_VMEM, size_t PAGE_SIZE>
PageEntry PageTable<TOTAL_VMEM, PAGE_SIZE>::createNew(size_t vAddr, size_t pAddr) {
	PageEntry entry(vAddr, pAddr);
	entries.insert(entry);
	return entry;
}

template<size_t TOTAL_VMEM, size_t PAGE_SIZE>
void PageTable<TOTAL_VMEM, PAGE_SIZE>::printSummary() const {
	printf("=== PageTable Summary ===\n");
	printf("  TOTAL_VMEM=%zu | PAGE_SIZE=%zu | PAGES=%zu\n",
		TOTAL_VMEM, PAGE_SIZE, TOTAL_VMEM / PAGE_SIZE);
	printf("  Entries: %zu\n", entries.size());
	for (const auto &e : entries) {
		printf("    vAddr=%zu pAddr=%zu\n", e.getVAddr(), e.getPAddr());
	}
}
