#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <thread>
#include <vector>

#include "pageEntry.hpp"
#include "fastSegTree.hpp"

template<size_t ENTRIES>
class TLB {
private:
	uint64_t timer;
	FastSegTree<ENTRIES> segTree;
	std::vector<PageEntry> entries;
	std::vector<std::thread::id> entryTids;
	std::map<std::pair<std::thread::id, size_t>, size_t> pageToIndex;
	mutable size_t tlbHits = 0;
	mutable size_t tlbMisses = 0;
	mutable size_t tlbEvictions = 0;
public:
	TLB();
	bool exist(const std::thread::id &tid, size_t pageNumber) const;
	PageEntry get(const std::thread::id &tid, size_t pageNumber);
	bool isFull() const;
	size_t removeOldest();
	void addPageEntry(const std::thread::id &tid, PageEntry newEntry);
	bool remove(const std::thread::id &tid, size_t pageNumber);

	void printSummary() const;
};

template<size_t ENTRIES>
TLB<ENTRIES>::TLB() : timer(0) {
	entries.reserve(ENTRIES);
	entryTids.reserve(ENTRIES);
	segTree.build();
}

template<size_t ENTRIES>
bool TLB<ENTRIES>::exist(const std::thread::id &tid, size_t pageNumber) const {
	bool found = pageToIndex.find({tid, pageNumber}) != pageToIndex.end();
	if (found) tlbHits++; else tlbMisses++;
	return found;
}

template<size_t ENTRIES>
PageEntry TLB<ENTRIES>::get(const std::thread::id &tid, size_t pageNumber) {
	auto it = pageToIndex.find({tid, pageNumber});
	if (it == pageToIndex.end()) {
		return PageEntry(-1, -1);
	}
	size_t idx = it->second;
	segTree.update(idx, ++timer);
	return entries[idx];
}

template<size_t ENTRIES>
bool TLB<ENTRIES>::isFull() const {
	return entries.size() >= ENTRIES;
}

template<size_t ENTRIES>
size_t TLB<ENTRIES>::removeOldest() {
	tlbEvictions++;
	size_t oldestIdx = (size_t)segTree.getMinIndex();
	PageEntry &oldest = entries[oldestIdx];
	std::thread::id &tid = entryTids[oldestIdx];

	auto it = pageToIndex.find({tid, oldest.getVAddr()});
	if (it != pageToIndex.end()) {
		pageToIndex.erase(it);
	}

	return oldestIdx;
}

template<size_t ENTRIES>
void TLB<ENTRIES>::addPageEntry(const std::thread::id &tid, PageEntry newEntry) {
	size_t idx;
	if (isFull()) {
		idx = removeOldest();
		entries[idx] = newEntry;
		entryTids[idx] = tid;
	} else {
		entries.push_back(newEntry);
		entryTids.push_back(tid);
		idx = entries.size() - 1;
	}
	pageToIndex[{tid, newEntry.getVAddr()}] = idx;
	segTree.update(idx, ++timer);
}

template<size_t ENTRIES>
bool TLB<ENTRIES>::remove(const std::thread::id &tid, size_t pageNumber) {
	auto it = pageToIndex.find({tid, pageNumber});
	if (it == pageToIndex.end()) {
		return false;
	}
	pageToIndex.erase(it);
	return true;
}

template<size_t ENTRIES>
void TLB<ENTRIES>::printSummary() const {
	printf("=== TLB Summary (size=%zu) ===\n", ENTRIES);
	printf("  Entries: %zu / %zu\n", entries.size(), ENTRIES);
	printf("  Hits: %zu | Misses: %zu | Evictions: %zu\n",
		tlbHits, tlbMisses, tlbEvictions);
	for (size_t i = 0; i < entries.size(); i++) {
		printf("    [%zu] thread=%zx vAddr=%zu pAddr=%zu\n",
			i, std::hash<std::thread::id>()(entryTids[i]),
			entries[i].getVAddr(), entries[i].getPAddr());
	}
}