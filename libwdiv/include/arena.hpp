

#pragma once
#include "config.hpp"

const size_t chunkSize = 16UL * 1024UL; // 16384 bytes
const size_t maxBlockSize = 640UL;
const size_t blockSizes = 14UL;
const size_t chunkArrayIncrement = 128UL;
const size_t stackSize = 100UL * 1024UL; // 100k
const size_t maxStackEntries = 32UL;

struct StackEntry
{
	char *data;
	size_t size;
	bool usedMalloc;
};

struct Block;
struct Chunk;


//  StringObject *str = strings[i];
// str->~StringObject();
// ARENA_FREE(str, sizeof(StringObject));
// void *p = ARENA_ALLOC(sizeof(StringObject));
// StringObject *obj = new (p) StringObject(value);

class BlockAllocator
{
public:
	BlockAllocator();
	~BlockAllocator();

	BlockAllocator(const BlockAllocator &) = delete;
	BlockAllocator &operator=(const BlockAllocator &) = delete;
	BlockAllocator(BlockAllocator &&) = delete;
	BlockAllocator &operator=(BlockAllocator &&) = delete;

	/// Allocate memory. This will use b2::Alloc if the size is larger than maxBlockSize.
	void *Allocate(size_t size);

	/// Free memory. This will use b2::Free if the size is larger than maxBlockSize.
	void Free(void *p, size_t size);

	void Clear();

	static BlockAllocator &instance()
	{
		static BlockAllocator allocator;
		return allocator;
	}

private:
	Chunk *m_chunks;
	size_t m_chunkCount;
	size_t m_chunkSpace;

	Block *m_freeLists[blockSizes];

	static size_t s_blockSizes[blockSizes];
	static uint8 s_blockSizeLookup[maxBlockSize + 1];
	static bool s_blockSizeLookupInitialized;
};

// This is a stack allocator used for fast per step allocations.
// You must nest allocate/free pairs. The code will assert
// if you try to interleave multiple allocate/free pairs.
class StackAllocator
{
public:
	StackAllocator();
	~StackAllocator();

	StackAllocator(const StackAllocator &) = delete;
	StackAllocator &operator=(const StackAllocator &) = delete;
	StackAllocator(StackAllocator &&) = delete;
	StackAllocator &operator=(StackAllocator &&) = delete;
	void *Allocate(size_t size);
	void Free(void *p);

	size_t GetMaxAllocation() const;

private:
	char m_data[stackSize];
	size_t m_index;

	size_t m_allocation;
	size_t m_maxAllocation;

	StackEntry m_entries[maxStackEntries];
	size_t m_entryCount;
};