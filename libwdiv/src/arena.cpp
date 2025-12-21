#include "arena.hpp"
#include <cstring>
#include <cstdlib>
#include <climits>

size_t BlockAllocator::s_blockSizes[blockSizes] =
	{
		16,	 // 0
		32,	 // 1
		64,	 // 2
		96,	 // 3
		128, // 4
		160, // 5
		192, // 6
		224, // 7
		256, // 8
		320, // 9
		384, // 10
		448, // 11
		512, // 12
		640, // 13
};
uint8 BlockAllocator::s_blockSizeLookup[maxBlockSize + 1];
bool BlockAllocator::s_blockSizeLookupInitialized;

struct Chunk
{
	size_t blockSize;
	Block *blocks;
};

struct Block
{
	Block *next;
};

BlockAllocator::BlockAllocator()
{
	assert(blockSizes < UCHAR_MAX);

	m_chunkSpace = chunkArrayIncrement;
	m_chunkCount = 0;
	m_chunks = (Chunk *)aAlloc(m_chunkSpace * sizeof(Chunk));

	std::memset(m_chunks, 0, m_chunkSpace * sizeof(Chunk));
	for (auto &list : m_freeLists)
	{
		list = nullptr;
	}

	if (s_blockSizeLookupInitialized == false)
	{
		size_t j = 0;
		for (size_t i = 1; i <= maxBlockSize; ++i)
		{
			assert(j < blockSizes);
			if (i <= s_blockSizes[j])
			{
				s_blockSizeLookup[i] = (uint8)j;
			}
			else
			{
				++j;
				s_blockSizeLookup[i] = (uint8)j;
			}
		}

		s_blockSizeLookupInitialized = true;
	}
}

BlockAllocator::~BlockAllocator()
{
	for (size_t i = 0; i < m_chunkCount; ++i)
	{
		aFree(m_chunks[i].blocks);
	}

	aFree(m_chunks);
}

void *BlockAllocator::Allocate(size_t size)
{
	if (size == 0)
		return NULL;

	assert(0 < size);

	if (size > maxBlockSize)
	{
		return aAlloc(size);
	}

	size_t index = s_blockSizeLookup[size];
	assert(0 <= index && index < blockSizes);

	if (m_freeLists[index])
	{
		Block *block = m_freeLists[index];
		m_freeLists[index] = block->next;
		return block;
	}
	else
	{
		if (m_chunkCount == m_chunkSpace)
		{
			Chunk *oldChunks = m_chunks;
			m_chunkSpace += chunkArrayIncrement;
			m_chunks = (Chunk *)aAlloc(m_chunkSpace * sizeof(Chunk));
			std::memcpy(m_chunks, oldChunks, m_chunkCount * sizeof(Chunk));
			std::memset(m_chunks + m_chunkCount, 0, chunkArrayIncrement * sizeof(Chunk));
			aFree(oldChunks);
		}

		Chunk *chunk = m_chunks + m_chunkCount;
		chunk->blocks = (Block *)aAlloc(chunkSize);
#if defined(_DEBUG)
		std::memset(chunk->blocks, 0xcd, chunkSize);
#endif
		size_t blockSize = s_blockSizes[index];
		chunk->blockSize = blockSize;
		size_t blockCount = chunkSize / blockSize;
		assert(blockCount * blockSize <= chunkSize);
		auto *base = reinterpret_cast<int8 *>(chunk->blocks);
		for (size_t i = 0; i < blockCount - 1; ++i)
		{
			Block *block = reinterpret_cast<Block *>(base + static_cast<ptrdiff_t>(blockSize) * i);
			Block *next = reinterpret_cast<Block *>(base + static_cast<ptrdiff_t>(blockSize) * (i + 1));
			block->next = next;
		}
		Block *last = reinterpret_cast<Block *>(base + static_cast<ptrdiff_t>(blockSize) * (blockCount - 1));
		last->next = NULL;

		m_freeLists[index] = chunk->blocks->next;
		++m_chunkCount;

		return chunk->blocks;
	}
}

void BlockAllocator::Free(void *p, size_t size)
{
	if (size == 0)
	{
		return;
	}

	assert(0 < size);

	if (size > maxBlockSize)
	{
		aFree(p);
		return;
	}

	size_t index = s_blockSizeLookup[size];
	assert(0 <= index && index < blockSizes);

#ifdef _DEBUG
	// Verify the memory address and size is valid.
	size_t blockSize = s_blockSizes[index];
	bool found = false;
	for (size_t i = 0; i < m_chunkCount; ++i)
	{
		Chunk *chunk = m_chunks + i;
		if (chunk->blockSize != blockSize)
		{
			assert((int8 *)p + blockSize <= (int8 *)chunk->blocks ||
				   (int8 *)chunk->blocks + chunkSize <= (int8 *)p);
		}
		else
		{
			if ((int8 *)chunk->blocks <= (int8 *)p && (int8 *)p + blockSize <= (int8 *)chunk->blocks + chunkSize)
			{
				found = true;
			}
		}
	}

	assert(found);

	std::memset(p, 0xfd, blockSize);
#endif

	Block *block = (Block *)p;
	block->next = m_freeLists[index];
	m_freeLists[index] = block;
}

void BlockAllocator::Clear()
{
	for (size_t i = 0; i < m_chunkCount; ++i)
	{
		aFree(m_chunks[i].blocks);
	}

	m_chunkCount = 0;
	std::memset(m_chunks, 0, m_chunkSpace * sizeof(Chunk));

	for (auto &list : m_freeLists)
	{
		list = nullptr;
	}
}
//********************************************************************** */

StackAllocator::StackAllocator()
{
	m_index = 0;
	m_allocation = 0;
	m_maxAllocation = 0;
	m_entryCount = 0;
}

StackAllocator::~StackAllocator()
{
	assert(m_index == 0);
	assert(m_entryCount == 0);
}

void *StackAllocator::Allocate(size_t size)
{
	assert(m_entryCount < maxStackEntries);

	StackEntry *entry = m_entries + m_entryCount;
	entry->size = size;
	if (m_index + size > stackSize)
	{
		entry->data = (char *)aAlloc(size);
		entry->usedMalloc = true;
	}
	else
	{
		entry->data = m_data + m_index;
		entry->usedMalloc = false;
		m_index += size;
	}

	m_allocation += size;
	m_maxAllocation = Max(m_maxAllocation, m_allocation);
	++m_entryCount;

	return entry->data;
}

void StackAllocator::Free(void *p)
{
	assert(m_entryCount > 0);
	StackEntry *entry = m_entries + m_entryCount - 1;
	assert(p == entry->data);
	if (entry->usedMalloc)
	{
		aFree(p);
	}
	else
	{
		m_index -= entry->size;
	}
	m_allocation -= entry->size;
	--m_entryCount;

	p = NULL;
}

size_t StackAllocator::GetMaxAllocation() const
{
	return m_maxAllocation;
}