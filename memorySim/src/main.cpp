#include "mainMemory.hpp"
#include "pageEntry.hpp"
#include "pageTable.hpp"
#include "tlb.hpp"
#include "mmu.hpp"
#include "String.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>

void testingMainMemory();
void testingPageTable();
void testingTLB();
void testingString();
void testingMMU();
void testingTLBFull();
void testingMMUThreads();
void testingParallel10s();

int main() {
	testingMainMemory();
	testingPageTable();
	testingTLB();
	testingTLBFull();
	testingMMU();
	testingMMUThreads();
	testingParallel10s();
	testingString();
	return 0;
}

void testingMainMemory(){
	printf("Testing Main Memory:\n");
	MainMemory<100, 10> memory;
	memory.printSummary();
	size_t address = memory.allocate(15);
	memory.printSummary();
	char buff[] = "Testing";
	printf("Writing: |%s| at %zu,%zu\n", buff, address, (size_t) 0);
	memory.writeInto(buff, address, 0, sizeof(buff));
	char oth[8];
	memory.readInto(oth, address, 2, sizeof(oth));
	printf("Readed: |%s| from %zu,%zu\n", oth, address, (size_t) 2);
}

void testingPageTable() {
	PageTable<1000, 10> table;

	table.printSummary();
	PageEntry invalid = table.getEntry(123);
	printf("Searching for 123: %zu, %zu\n", invalid.getVAddr(), invalid.getPAddr());

	table.createNew(12, 45);
	table.printSummary();
	PageEntry valid = table.getEntry(12);
	printf("Searching for 12: %zu, %zu\n", valid.getVAddr(), valid.getPAddr());
}

void testingTLB() {
	printf("\n=== Testing TLB ===\n");

	TLB<4> tlb;
	printf("-- Empty TLB --\n");
	tlb.printSummary();

	printf("\n-- Filling TLB (4 entries) --\n");
	tlb.addPageEntry(PageEntry(0x1000, 0xA000));
	tlb.addPageEntry(PageEntry(0x2000, 0xB000));
	tlb.addPageEntry(PageEntry(0x3000, 0xC000));
	tlb.addPageEntry(PageEntry(0x4000, 0xD000));
	tlb.printSummary();
	printf("  isFull: %d (expected 1)\n", tlb.isFull());

	printf("\n-- exist/get --\n");
	printf("  exist(%zu): %d (expected 1)\n", (size_t)0x1000, tlb.exist(0x1000));
	printf("  exist(%zu): %d (expected 0)\n", (size_t)0x9999, tlb.exist(0x9999));

	PageEntry e = tlb.get(0x2000);
	printf("  get(%zu): vAddr=%zu pAddr=%zu (expected %zu %zu)\n",
		(size_t)0x2000, e.getVAddr(), e.getPAddr(), (size_t)0x2000, (size_t)0xB000);

	PageEntry miss = tlb.get(0x9999);
	printf("  get(%zu) on miss: vAddr=%zu pAddr=%zu (expected -1 -1)\n",
		(size_t)0x9999, miss.getVAddr(), miss.getPAddr());

	printf("\n-- Adding past capacity (eviction) --\n");
	printf("  Order by timer: %zu(t1), %zu(t2->t5 on get), %zu(t3), %zu(t4)\n",
		(size_t)0x1000, (size_t)0x2000, (size_t)0x3000, (size_t)0x4000);
	printf("  Oldest untouched is %zu (timer=1)\n", (size_t)0x1000);
	tlb.addPageEntry(PageEntry(0x5000, 0xE000));
	tlb.printSummary();
  printf("  exist(%zu): %d (expected 0 - evicted)\n", (size_t)0x1000, tlb.exist(0x1000));
  printf("  exist(%zu): %d (expected 1 - newly added)\n", (size_t)0x5000, tlb.exist(0x5000));
}

void testingTLBFull() {
	printf("\n=== Testing TLB Full Behavior ===\n");

	TLB<3> tlb;
	size_t e1 = 0x100, e2 = 0x200, e3 = 0x300;
	size_t e4 = 0x400, e5 = 0x500, e6 = 0x600;

	printf("-- Fill TLB(3) to capacity --\n");
	tlb.addPageEntry(PageEntry(e1, 1));
	tlb.addPageEntry(PageEntry(e2, 2));
	tlb.addPageEntry(PageEntry(e3, 3));
	printf("  isFull: %d (expected 1)\n", tlb.isFull());
	printf("  exist(%zu): %d exist(%zu): %d exist(%zu): %d\n",
		e1, tlb.exist(e1), e2, tlb.exist(e2), e3, tlb.exist(e3));

	printf("\n-- Evict oldest (e1) by adding e4 --\n");
	tlb.addPageEntry(PageEntry(e4, 4));
	printf("  exist(%zu): %d (expected 0 - evicted)\n", e1, tlb.exist(e1));
	printf("  exist(%zu): %d (expected 1 - new)\n", e4, tlb.exist(e4));
	printf("  exist(%zu): %d exist(%zu): %d (expected 1 1)\n",
		e2, tlb.exist(e2), e3, tlb.exist(e3));

	printf("\n-- Refresh e2, then evict oldest (e3) --\n");
	tlb.get(e2);
	tlb.addPageEntry(PageEntry(e5, 5));
	printf("  exist(%zu): %d (expected 0 - oldest untouched)\n", e3, tlb.exist(e3));
	printf("  exist(%zu): %d (expected 1 - refreshed)\n", e2, tlb.exist(e2));
	printf("  exist(%zu): %d (expected 1 - new)\n", e5, tlb.exist(e5));

	printf("\n-- Multiple successive evictions --\n");
	tlb.addPageEntry(PageEntry(e6, 6));
	printf("  after e6 added, exist(e4): %d (expected 0 - evicted)\n", tlb.exist(e4));
	printf("  exist(e6): %d (expected 1)\n", tlb.exist(e6));
	printf("  exist(e2): %d exist(e5): %d (expected 1 1)\n",
		tlb.exist(e2), tlb.exist(e5));

	printf("\n-- Verify entries still readable after eviction chain --\n");
	PageEntry g2 = tlb.get(e2);
	printf("  get(e2): pAddr=%zu (expected 2)\n", g2.getPAddr());
	PageEntry g5 = tlb.get(e5);
	printf("  get(e5): pAddr=%zu (expected 5)\n", g5.getPAddr());
	PageEntry g6 = tlb.get(e6);
	printf("  get(e6): pAddr=%zu (expected 6)\n", g6.getPAddr());

	tlb.printSummary();
}

void testingMMU() {
	printf("\n=== Testing MMU ===\n");

	auto memory = std::make_shared<MainMemory<1000, 10>>();
	MMU<1000, 10, 1000, 4> mmu(memory);

	printf("-- Allocate + Write / Read --\n");
	size_t vAddr = 0x1000;
	mmu.allocate(vAddr, 5);
	char written[] = "ABCDE";
	mmu.write(written, vAddr, 5);
	char readback[6] = {0};
	mmu.read(readback, vAddr, 5);
	printf("  read: %s (expected ABCDE)\n", readback);

	printf("\n-- Read from unmapped address (page fault) --\n");
	char buf[4] = {0};
	int ret = mmu.read(buf, 0x2000, 1);
	printf("  read(0x2000): %d (expected -1)\n", ret);

	printf("\n-- Cross-page allocation (size > FRAME_SIZE) --\n");
	size_t bigVAddr = 0x3000;
	mmu.allocate(bigVAddr, 25);
	char bigSrc[] = "Hello from multi-page!";
	mmu.write(bigSrc, bigVAddr, sizeof(bigSrc));
	char bigDst[23] = {0};
	mmu.read(bigDst, bigVAddr, sizeof(bigSrc));
	printf("  read: %s (expected Hello from multi-page!)\n", bigDst);

	printf("\n-- Free and reallocate --\n");
	mmu.free(vAddr);
	mmu.allocate(0x5000, 5);
	char rewrite[] = "VWXYZ";
	mmu.write(rewrite, 0x5000, 5);
	char reread[6] = {0};
	mmu.read(reread, 0x5000, 5);
	printf("  read after realloc: %s (expected VWXYZ)\n", reread);

	printf("\n-- Page fault counter --\n");
	printf("  page faults: %zu (expected 0)\n", mmu.getPageFaults());
	mmu.read(buf, 0x6000, 1);
	printf("  after unmapped read: %zu (expected 0 - no alloc attempt, no fault)\n", mmu.getPageFaults());

	printf("\n-- printSummary --\n");
	mmu.printSummary();
}

void testingMMUThreads() {
	printf("\n=== Testing MMU Multi-thread (same vAddr, per-thread pt) ===\n");

	auto memory = std::make_shared<MainMemory<10000, 10>>();
	auto mmu = std::make_shared<MMU<10000, 10, 10000, 4>>(memory);

	size_t sameVAddr = 0x1000;
	bool waitFlag = true;

	std::thread t1([mmu, sameVAddr, &waitFlag]() {
		while(waitFlag) {}
		mmu->allocate(sameVAddr, 8);
		const char *data = "Thread1!";
		mmu->write((void*)data, sameVAddr, 8);
		char buf[9] = {0};
		std::this_thread::sleep_for(std::chrono::seconds(1));
		mmu->read(buf, sameVAddr, 8);
		printf("  Thread1: read back \"%s\" from vAddr=0x%zx\n", buf, sameVAddr);
		mmu->free(sameVAddr);
	});

	std::thread t2([mmu, sameVAddr, &waitFlag]() {
		while(waitFlag) {}
		mmu->allocate(sameVAddr, 8);
		const char *data = "Thread2!";
		mmu->write((void*)data, sameVAddr, 8);
		char buf[9] = {0};
		std::this_thread::sleep_for(std::chrono::seconds(1));
		mmu->read(buf, sameVAddr, 8);
		printf("  Thread2: read back \"%s\" from vAddr=0x%zx\n", buf, sameVAddr);
		mmu->free(sameVAddr);
	});

	waitFlag = false;
	t1.join();
	t2.join();

	mmu->printSummary();
}

void testingParallel10s() {
	printf("\n=== Testing 10s Parallel MMU Stress (Memory Exhaustion) ===\n");
	printf("  TOTAL_MEM=%zu KB (%zu frames de %zu KB) | TOTAL_VMEM=%zu KB (%zu paginas)\n\n",
		(size_t)65536/1024, (size_t)65536/8192, (size_t)8192/1024,
		(size_t)1048576/1024, (size_t)1048576/8192);

	auto memory = std::make_shared<MainMemory<65536, 8192>>();
	auto mmu = std::make_shared<MMU<65536, 8192, 1048576, 8>>(memory);

	mmu->clearPageFaults();
	std::atomic<bool> stop(false);
	std::atomic<size_t> allocs1(0), allocs2(0);
	std::atomic<size_t> faults1(0), faults2(0);
	std::atomic<size_t> errors1(0), errors2(0);

	std::thread t1([mmu, &stop, &allocs1, &faults1, &errors1]() {
		while (!stop.load()) {
			size_t addrs[128];
			size_t count = 0;
			for (size_t i = 0; i < 128 && !stop.load(); i++) {
				size_t vAddr = 0x100000 + i * 8192;
				int ret = mmu->allocate(vAddr, 512);
				if (ret == 0) {
					mmu->write((void*)"T1-data", vAddr, 7);
					addrs[count++] = vAddr;
				} else {
					faults1++;
				}
			}
			allocs1 += count;
			for (size_t j = 0; j < count && !stop.load(); j++) {
				size_t vAddr = addrs[j];
				char buf[8] = {0};
				mmu->read(buf, vAddr, 7);
				if (std::strcmp(buf, "T1-data") != 0) {
					errors1++;
				}
				mmu->free(vAddr);
			}
		}
	});

	std::thread t2([mmu, &stop, &allocs2, &faults2, &errors2]() {
		while (!stop.load()) {
			size_t addrs[128];
			size_t count = 0;
			for (size_t i = 0; i < 128 && !stop.load(); i++) {
				size_t vAddr = 0x200000 + i * 8192;
				int ret = mmu->allocate(vAddr, 512);
				if (ret == 0) {
					mmu->write((void*)"T2-data", vAddr, 7);
					addrs[count++] = vAddr;
				} else {
					faults2++;
				}
			}
			allocs2 += count;
			for (size_t j = 0; j < count && !stop.load(); j++) {
				size_t vAddr = addrs[j];
				char buf[8] = {0};
				mmu->read(buf, vAddr, 7);
				if (std::strcmp(buf, "T2-data") != 0) {
					errors2++;
				}
				mmu->free(vAddr);
			}
		}
	});

	std::this_thread::sleep_for(std::chrono::seconds(10));
	stop.store(true);
	t1.join();
	t2.join();

	size_t totalFaults = mmu->getPageFaults();

	printf("\n  Thread1: %zu allocs, %zu page faults, %zu data errors\n",
		allocs1.load(), faults1.load(), errors1.load());
	printf("  Thread2: %zu allocs, %zu page faults, %zu data errors\n",
		allocs2.load(), faults2.load(), errors2.load());
	printf("  Total: %zu allocs | %zu page faults | %zu errors\n\n",
		allocs1.load() + allocs2.load(), totalFaults,
		errors1.load() + errors2.load());

	mmu->printSummary();

	printf("\n  Result: %s\n",
		errors1.load() == 0 && errors2.load() == 0 ? "PASS (data intact)" : "FAIL (corruption)");
}

void testingString() {
	printf("\n=== Testing String with MMU ===\n");

	auto memory = std::make_shared<MainMemory<1000, 10>>();
	auto mmu = std::make_shared<MMU<1000, 10, 1000, 4>>(memory);

	String<1000, 10, 1000, 4> str(mmu, 50);

	printf("  capacity: %zu\n", str.capacity());
	printf("  length: %zu\n", str.length());
	str.write("Hello");

	printf("  after sets, length: %zu\n", str.length());
	printf("  chars: ");
	for (size_t i = 0; i < str.length(); i++) {
		putchar(str.get(i));
	}
	printf("\n");

	str.append(" World!");
	printf("  after append, length: %zu\n", str.length());
	printf("  string: \"");
	str.print();
	printf("\"\n");

	mmu->printSummary();
}