#pragma once
#include <cstddef>
#include <functional>

class PageEntry {
private:
	size_t vAddr;
	size_t pAddr;
public:
	PageEntry();
	PageEntry(size_t vAddr, size_t pAddr);

	size_t getVAddr() const;
	size_t getPAddr() const;

	bool operator==(const PageEntry &o) const;
};

template<>
struct std::hash<PageEntry> {
	size_t operator()(const PageEntry &e) const {
		return std::hash<size_t>()(e.getVAddr());
	}
};
