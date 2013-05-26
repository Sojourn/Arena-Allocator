#include "Arena.h"
#include <iostream>

struct ArenaInitList_t
{
	const char *name;
	size_t capacity;
};

static const ArenaInitList_t _initList[ArenaTag_e::COUNT] =
{
#define ARENA(name, capacity) {#name, (capacity)},
#include "ArenaDeclarations.h"
#undef ARENA
};

ArenaManager &ArenaManager::instance()
{
	static ArenaManager manager;
	return manager;
}

ArenaManager::ArenaManager()
{
}

ArenaManager::~ArenaManager()
{
}

void ArenaManager::Init()
{
	ArenaManager &manager = ArenaManager::instance();
	size_t offset = 0;
	manager._capacity = 0;

	for(size_t i = 0; i < (size_t) ArenaTag_e::COUNT; i++)
	{
		manager._arenas[i].base = manager._arenas[i].top = nullptr;
		manager._arenas[i].capacity = _initList[i].capacity;
		manager._capacity += _initList[i].capacity;
	}
	manager._base = new uint8_t[manager._capacity];

	for(size_t i = 0; i < (size_t) ArenaTag_e::COUNT; i++)
	{
		manager._arenas[i].base = manager._arenas[i].top = manager._base + offset;
	}
}

void ArenaManager::Deinit()
{
	const ArenaManager &manager = ArenaManager::instance();
	delete [] manager._base;
}

const char *ArenaManager::GetName(ArenaTag_e tag)
{
	return _initList[(size_t) tag].name;
}

size_t ArenaManager::Capacity(ArenaTag_e tag)
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager._arenas[(size_t) tag].capacity;
}

size_t ArenaManager::TotalCapacity()
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager._capacity;
}

size_t ArenaManager::Used(ArenaTag_e tag)
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager._arenas[(size_t) tag].top - manager._arenas[(size_t) tag].base;
}

size_t ArenaManager::TotalUsed()
{
	size_t totalUsed = 0;
	for(size_t i = 0; i < (size_t) ArenaTag_e::COUNT; i++)
	{
		totalUsed += Used((ArenaTag_e) i);
	}

	return totalUsed;
}

size_t ArenaManager::Free(ArenaTag_e tag)
{
	return ArenaManager::instance()._arenas[(size_t) tag].capacity - Used(tag);
}

size_t ArenaManager::TotalFree()
{
	size_t totalFree = 0;
	for(size_t i = 0; i < (size_t) ArenaTag_e::COUNT; i++)
	{
		totalFree += Free((ArenaTag_e) i);
	}

	return totalFree;
}

ArenaAllocator::ArenaAllocator(ArenaTag_e tag) :
	_tag(tag)
{
	_arena = &ArenaManager::instance()._arenas[(size_t) tag];
	_oldTop = _arena->top;
}

ArenaAllocator::~ArenaAllocator()
{
	_arena->top = _oldTop;
}

uint32_t CalculatePadding(const uint8_t *ptr, const uint32_t alignment)
{
	uint32_t mask = alignment - 1;
	uint32_t misalignment = ((uint32_t) ptr) & mask;
	return (misalignment > 0) ? (alignment - misalignment) : 0;
}

void *ArenaAllocator::Allocate(const uint32_t size, uint32_t const alignment)
{
	uint8_t *base;
	uint8_t *alignedBase;
	uint8_t *sizePtr8;
	uint32_t *sizePtr32;
	uint32_t padding;
	uint32_t allocationSize;
	uint32_t requiredCapacity;

	// Align the starting address
	base = _arena->top;
	padding = CalculatePadding(base, alignment);
	alignedBase = base + padding;

	// Allocate if the allocation can be satisfied
	allocationSize = padding + size + sizeof(uint32_t);
	requiredCapacity = (_arena->top - _arena->base) + allocationSize;
	if(requiredCapacity > _arena->capacity)
	{
		return nullptr;
	}
	else
	{
		_arena->top += allocationSize;
	}

	// Record the size of the allocation in the frame
	sizePtr8 = alignedBase + size;
	sizePtr32 = (uint32_t*) sizePtr8;
	*sizePtr32 = allocationSize;

	assert(padding < alignment);
	assert((alignedBase - base) == padding);
	assert((base + allocationSize) == _arena->top);
	assert((alignedBase + size) == (_arena->top - 4));
	assert((sizePtr8 + 4) == _arena->top);

	return alignedBase;
}

void ArenaAllocator::Free(void *)
{
	uint32_t allocationSize;
	uint8_t *prevTop;
	
	allocationSize = *((uint32_t*) (_arena->top - 4));
	prevTop = _arena->top - allocationSize;

	assert(_arena->base <= prevTop);

	_arena->top = prevTop;
}

void ArenaAllocator::DumpArena() const
{
	const ArenaManager &manager = ArenaManager::instance();
	size_t allocationSize;
	uint8_t *frameTop;
	uint8_t *frameBottom;
	uint32_t *sizePtr;

	std::cout << "[" << manager.GetName(_tag) << " Arena]" << std::endl;
	std::cout << "Capacity: " << manager.Capacity(_tag) << std::endl;
	std::cout << "Free: " << manager.Free(_tag) << std::endl;
	std::cout << "Used: " << manager.Used(_tag) << std::endl;
	std::cout << "Frames: " << std::endl;

	frameTop = _arena->top;
	while(frameTop > _arena->base)
	{
		sizePtr = (uint32_t*) (frameTop - 4);
		allocationSize = *sizePtr;
		frameBottom = frameTop - allocationSize;

		std::cout << "    " << allocationSize << std::endl;
		frameTop = frameBottom;
	}

	std::cout << std::endl;
}