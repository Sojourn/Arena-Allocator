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
	manager.capacity = 0;

	for(size_t i = 0; i < (size_t) ArenaTag_e::COUNT; i++)
	{
		manager.arenas[i].base = manager.arenas[i].top = nullptr;
		manager.arenas[i].capacity = _initList[i].capacity;
		manager.capacity += _initList[i].capacity;
	}
	manager.base = new uint8_t[manager.capacity];

	for(size_t i = 0; i < (size_t) ArenaTag_e::COUNT; i++)
	{
		manager.arenas[i].base = manager.arenas[i].top = manager.base + offset;
	}
}

void ArenaManager::Deinit()
{
	const ArenaManager &manager = ArenaManager::instance();
	delete [] manager.base;
}

const char *ArenaManager::GetName(ArenaTag_e tag)
{
	return _initList[(size_t) tag].name;
}

size_t ArenaManager::Capacity(ArenaTag_e tag)
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager.arenas[(size_t) tag].capacity;
}

size_t ArenaManager::TotalCapacity()
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager.capacity;
}

size_t ArenaManager::Used(ArenaTag_e tag)
{
	const ArenaManager &manager = ArenaManager::instance();
	return manager.arenas[(size_t) tag].top - manager.arenas[(size_t) tag].base;
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
	return ArenaManager::instance().arenas[(size_t) tag].capacity - Used(tag);
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

void ArenaManager::DumpArena(ArenaTag_e tag)
{
	std::cout << "[" << GetName(tag) << " Arena]" << std::endl;
	std::cout << "Capacity: " << Capacity(tag) << std::endl;
	std::cout << "Free: " << Free(tag) << std::endl;
	std::cout << "Used: " << Used(tag) << std::endl;
	std::cout << std::endl;
}

void ArenaAllocator::Init(ArenaTag_e tag)
{
	arena = &ArenaManager::instance().arenas[(size_t) tag];
	oldTop = arena->top;
	lastAllocSize = 0;
}

void ArenaAllocator::Deinit()
{
	arena->top = oldTop;
}

ArenaAllocator::ArenaAllocator(ArenaTag_e tag)
{
	Init(tag);
}

ArenaAllocator::ArenaAllocator() :
	arena(nullptr),
	oldTop(nullptr)
{
}

ArenaAllocator::~ArenaAllocator()
{
	Deinit();
}

void *ArenaAllocator::Allocate(size_t size)
{
	size_t nextUsed = (arena->top + size) - arena->base;
	if(nextUsed > arena->capacity)
		return nullptr;

	uint8_t *ptr = arena->top;
	arena->top += size;
	lastAllocSize = size;
	return ptr;
}

void ArenaAllocator::Free(void *ptr)
{
	// TODO: Store the last allocation size at the top of an allocation

	uint8_t *prevTop = (uint8_t*) ptr;
	assert((prevTop + lastAllocSize) == arena->top);

	arena->top -= lastAllocSize;
	lastAllocSize = 0;
}