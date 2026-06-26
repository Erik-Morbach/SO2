#include "pageEntry.hpp"

PageEntry::PageEntry() : vAddr(0), pAddr(0) {}

PageEntry::PageEntry(size_t vAddr, size_t pAddr) : vAddr(vAddr), pAddr(pAddr) {}

size_t PageEntry::getVAddr() const {
	return vAddr;
}

size_t PageEntry::getPAddr() const {
	return pAddr;
}

// Só queremos validar o endereçamento virtual
bool PageEntry::operator==(const PageEntry &o) const {
	return vAddr == o.vAddr;
}
