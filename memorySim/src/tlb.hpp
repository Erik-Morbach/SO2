#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <vector>

#include "pageEntry.hpp"
#include "fastSegTree.hpp"

template<size_t ENTRIES>
class TLB {
private:
	uint64_t timer;
	FastSegTree<ENTRIES> segTree;
	std::vector<PageEntry> entries;
	std::unordered_map<size_t, size_t> pageToIndex;
	mutable size_t tlbHits = 0;
	mutable size_t tlbMisses = 0;
	mutable size_t tlbEvictions = 0;
public:
	TLB();
	bool exist(size_t pageNumber) const;
	PageEntry get(size_t pageNumber);
	bool isFull() const;
	size_t removeOldest();
	void addPageEntry(PageEntry newEntry);
	bool remove(size_t pageNumber);

	void printSummary() const;
};

	template<size_t ENTRIES>
	TLB<ENTRIES>::TLB() : timer(0) {
		entries.reserve(ENTRIES);
		segTree.build();
	}

	template<size_t ENTRIES>
	bool TLB<ENTRIES>::exist(size_t pageNumber) const {
		bool found = pageToIndex.find(pageNumber) != pageToIndex.end();
		if (found) tlbHits++; else tlbMisses++;
		return found;
	}

	template<size_t ENTRIES>
	PageEntry TLB<ENTRIES>::get(size_t pageNumber) {
		auto it = pageToIndex.find(pageNumber);
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

		auto it = pageToIndex.find(oldest.getVAddr());
		if (it != pageToIndex.end()) {
			pageToIndex.erase(it);
		}

		return oldestIdx;
	}

	template<size_t ENTRIES>
	void TLB<ENTRIES>::addPageEntry(PageEntry newEntry) {
		size_t idx;
		if (isFull()) {
			idx = removeOldest();
			entries[idx] = newEntry;
		} else {
			entries.push_back(newEntry);
			idx = entries.size() - 1;
		}
		pageToIndex[newEntry.getVAddr()] = idx;
		segTree.update(idx, ++timer);
	}

	template<size_t ENTRIES>
	bool TLB<ENTRIES>::remove(size_t pageNumber) {
		auto it = pageToIndex.find(pageNumber);
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
			printf("    [%zu] vAddr=%zu pAddr=%zu\n",
				i, entries[i].getVAddr(), entries[i].getPAddr());
		}
	}