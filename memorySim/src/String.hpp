#pragma once
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include "mmu.hpp"

template<size_t TOTAL_MEM, size_t FRAME_SIZE, size_t TOTAL_VMEM, size_t TLB_ENTRIES>
class String {
private:
	std::shared_ptr<MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>> mmu;
	size_t cap;
	size_t len;

public:
	String(std::shared_ptr<MMU<TOTAL_MEM, FRAME_SIZE, TOTAL_VMEM, TLB_ENTRIES>> mmu, size_t capacity)
		: mmu(mmu), cap(capacity), len(0) {
		mmu->allocate((size_t)this, capacity);
	}

	~String() {
		mmu->free((size_t)this);
	}

	void set(size_t index, char c) {
		if (index >= cap) return;
		mmu->write(&c, (size_t)this + index, 1);
		if (index >= len) len = index + 1;
	}

	char get(size_t index) {
		if (index >= cap) return 0;
		char c;
		mmu->read(&c, (size_t)this + index, 1);
		return c;
	}

	void append(char c) {
		if (len >= cap) return;
		set(len, c);
	}

	void append(const char *str) {
		for (size_t i = 0; str[i] != '\0' && len < cap; i++) {
			append(str[i]);
		}
	}

	void write(const char *src) {
		size_t n = strlen(src);
		mmu->write((void*)src, (size_t)this, n);
		len = n;
	}

	void print() {
		char *response = (char*) malloc(len + 1);
		mmu->read(response, (size_t)this, len);
		response[len] = '\0';
		printf("%s", response);
		free(response);
	}

	size_t capacity() const { return cap; }
	size_t length() const { return len; }
};
