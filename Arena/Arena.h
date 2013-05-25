#ifndef HEAP_H
#define HEAP_H

#include <cstdint>
#include <cassert>

/*
TODO:
- Hide ArenaManager's constructor
- Aligned memory allocations
- Store the last allocation size at the top of an allocation
- Refactor variable names to use hungarian notation
- Add 'Get' to ArenaManager's getter functions
- Add a current arena (arena stack) so the allocating arena can be asserted
- Add comments
*/

enum class ArenaTag_e : uint32_t
{
#define ARENA(name, capacity) name##_Arena,
#include "ArenaDeclarations.h"
#undef ARENA
	COUNT
};

struct Arena_t
{
	uint8_t* base;
	uint8_t* top;
	size_t   capacity;
};

class NoCopyAssign
{
protected:
	NoCopyAssign() {}

private:
	NoCopyAssign(const NoCopyAssign &);
	const NoCopyAssign &operator=(const NoCopyAssign &);
};

class ArenaManager : public NoCopyAssign
{
	friend class ArenaAllocator;
public:
	static void Init();
	static void Deinit();

	static const char *GetName(ArenaTag_e tag);

	static size_t Capacity(ArenaTag_e tag);
	static size_t TotalCapacity();

	static size_t Used(ArenaTag_e tag);
	static size_t TotalUsed();

	static size_t Free(ArenaTag_e tag);
	static size_t TotalFree();

	static void DumpArena(ArenaTag_e tag);

private:
	static ArenaManager &instance();
	ArenaManager();
	~ArenaManager();

	Arena_t arenas[ArenaTag_e::COUNT];
	uint8_t *base;
	size_t capacity;
};

class ArenaAllocator : public NoCopyAssign
{
	friend class ArenaManager;
public:
	ArenaAllocator(ArenaTag_e tag);
	~ArenaAllocator();

	void *Allocate(size_t size);
	void Free(void *ptr);

private:
	ArenaAllocator();
	void Init(ArenaTag_e tag);
	void Deinit();

	Arena_t *arena;
	uint8_t* oldTop;
	size_t lastAllocSize;
};

#endif // HEAP_H