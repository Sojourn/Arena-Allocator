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
		manager._arenas[i].topAllocator = nullptr;
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

size_t ArenaManager::GetCapacity(ArenaTag_e tag)
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager._arenas[(size_t) tag].capacity;
}

size_t ArenaManager::GetTotalCapacity()
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager._capacity;
}

size_t ArenaManager::GetUsed(ArenaTag_e tag)
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager._arenas[(size_t) tag].top - manager._arenas[(size_t) tag].base;
}

size_t ArenaManager::GetTotalUsed()
{
	size_t totalUsed = 0;
	for(size_t i = 0; i < (size_t) ArenaTag_e::COUNT; i++)
	{
		totalUsed += GetUsed((ArenaTag_e) i);
	}

	return totalUsed;
}

size_t ArenaManager::GetFree(ArenaTag_e tag)
{
	return ArenaManager::instance()._arenas[(size_t) tag].capacity - GetUsed(tag);
}

size_t ArenaManager::GetTotalFree()
{
	size_t totalFree = 0;
	for(size_t i = 0; i < (size_t) ArenaTag_e::COUNT; i++)
	{
		totalFree += GetFree((ArenaTag_e) i);
	}

	return totalFree;
}

ArenaAllocator::ArenaAllocator(ArenaTag_e tag, bool persist) :
	_tag(tag),
	_persist(persist)
{
	_arena = &ArenaManager::instance()._arenas[(size_t) tag];
	_parentAllocator = _arena->topAllocator;
	_arena->topAllocator = this;
	_oldTop = _arena->top;
}

ArenaAllocator::~ArenaAllocator()
{
	if(!_persist)
	{
		assert(_arena->topAllocator == this);
		_arena->top = _oldTop;
	}

	_arena->topAllocator = _parentAllocator;
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

	assert(_arena->topAllocator == this);

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

	// Verify the allocation frame
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
	
	assert(_arena->topAllocator == this);

	allocationSize = *((uint32_t*) (_arena->top - 4));
	prevTop = _arena->top - allocationSize;

	assert(_oldTop <= prevTop);
	assert(_arena->base <= prevTop);

	_arena->top = prevTop;
}

const char *ArenaAllocator::GetName() const
{
	return ArenaManager::GetName(_tag);
}

size_t ArenaAllocator::GetCapacity() const
{
	return _arena->capacity;
}

size_t ArenaAllocator::GetUsed() const
{
	return _arena->top - _arena->base;
}

size_t ArenaAllocator::GetFree() const
{
	return _arena->capacity - GetUsed();
}

void ArenaAllocator::DumpArena() const
{
	const ArenaManager &manager = ArenaManager::instance();
	size_t allocationSize;
	uint8_t *frameTop;
	uint8_t *frameBottom;
	uint32_t *sizePtr;

	std::cout << "[" << manager.GetName(_tag) << " Arena]" << std::endl;
	std::cout << "Capacity: " << manager.GetCapacity(_tag) << std::endl;
	std::cout << "Free: " << manager.GetFree(_tag) << std::endl;
	std::cout << "Used: " << manager.GetUsed(_tag) << std::endl;
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