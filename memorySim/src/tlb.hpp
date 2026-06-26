#pragma once
#include <cstddef>

#include "pageEntry.hpp"

class TLB {
public:
	bool exist(size_t pageNumber);
	PageEntry get(size_t pageNumber);
	bool isFull();
	bool removeOldest();
	void addPageEntry(PageEntry newEntry);
};