#ifndef HEAP_H
#define HEAP_H

#include <cstdint>
#include <cassert>

/*
TODO:
- Align the frame metadata
- Add comments
- Add an option to specify the arena sizes dynamically
- Add the option to resize an existing arena (through a page freelist)
- Test on other platforms
- Add alignment to the arena frame size
*/

class NoCopyAssign;
class ArenaManager;
class ArenaAllocator;

enum class ArenaTag_e : uint32_t
{
#define ARENA(name, capacity) name##_Arena,
#include "ArenaDeclarations.h"
#undef ARENA
	COUNT
};

struct Arena_t
{
	const ArenaAllocator *topAllocator;
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

	static size_t GetCapacity(ArenaTag_e tag);
	static size_t GetTotalCapacity();

	static size_t GetUsed(ArenaTag_e tag);
	static size_t GetTotalUsed();

	static size_t GetFree(ArenaTag_e tag);
	static size_t GetTotalFree();

	static void DumpArena(ArenaTag_e tag);

private:
	static ArenaManager &instance();
	ArenaManager();
	~ArenaManager();

	Arena_t   _arenas[ArenaTag_e::COUNT];
	uint8_t * _base;
	size_t    _capacity;
}; 

class ArenaAllocator : public NoCopyAssign
{
	friend class ArenaManager;
public:
	ArenaAllocator(ArenaTag_e tag, bool persist = false);
	~ArenaAllocator();

	void *Allocate(const uint32_t size, const uint32_t alignment = 4);
	
	// We don't actually need the pointer; just here for symmetry.
	void Free(void *ptr = nullptr);
	
	const char *GetName() const;
	size_t GetCapacity() const;
	size_t GetUsed() const;
	size_t GetFree() const;
	void DumpArena() const;

private:
	const ArenaTag_e      _tag;
	const bool            _persist;
	const ArenaAllocator *_parentAllocator;
	Arena_t              *_arena;
	uint8_t              *_oldTop;
};

#endif // HEAP_H