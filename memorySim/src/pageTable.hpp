#pragma once
#include <cstddef>
#include <unordered_set>

#include "pageEntry.hpp"

template<size_t TOTAL_VMEM, size_t PAGE_SIZE>
class PageTable {
private:
	//adicionar/remover PageEntries fica facil e performatico
	std::unordered_set<PageEntry> entries;
public:
	PageEntry getEntry(size_t vAddr);
	PageEntry createNew(size_t vAddr, size_t pAddr);
};