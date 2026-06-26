#pragma once
#include <cstddef>
#include <map>
#include <unordered_map>

#include "pageEntry.hpp"
#include "fastSegTree.hpp"

template<size_t ENTRIES>
class TLB {
private:
	uint64_t timer;
	FastSegTree<ENTRIES> segTree;

	std::vector<PageEntry> entries;

	std::map<int, std::unordered_map<size_t, size_t>> pageMapper;
	size_t pageNumberAndPidToIndex(size_t pageNumber);
public:
	bool exist(size_t pageNumber);
	PageEntry get(size_t pageNumber);
	bool isFull();
	bool removeOldest();
	void addPageEntry(PageEntry newEntry);
};