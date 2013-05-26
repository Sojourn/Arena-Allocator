#include <iostream>
#include <cstdlib>

#include "Arena.h"

void binaryDepthTest(size_t depth, uint8_t *data);

int main(int argc, char **argv)
{
	ArenaManager::Init();
	{
		//{
		//	uint8_t data = 0;
		//	binaryDepthTest(15, &data);
		//	std::cout << (size_t) data << std::endl;
		//}
		{
			ArenaAllocator arena(ArenaTag_e::DumpTest_Arena);
			void* ptr1 = arena.Allocate(1, 1);
			void* ptr2 = arena.Allocate(2, 2);
			void* ptr3 = arena.Allocate(3, 4);
			void* ptr4 = arena.Allocate(4, 8);
			void* ptr5 = arena.Allocate(5, 16);

			arena.Free();
			arena.Free();
			arena.Free();
			arena.Free();
			arena.Free(ptr1);

			arena.DumpArena();
		}

	}
	ArenaManager::Deinit();

	std::system("pause");
	return 0;
}

void binaryDepthTest(size_t depth, uint8_t *data)
{
	ArenaAllocator alloc(ArenaTag_e::RecursiveTest_Arena);
	uint8_t *childData[2];

	*data += 1;
	if(depth == 0)
		return;

	childData[0] = (uint8_t*) alloc.Allocate(1, 1);
	childData[1] = (uint8_t*) alloc.Allocate(1, 1);
	if(childData[0] == nullptr || childData[1] == nullptr)
		return;
	
	*childData[0] = 0;
	*childData[1] = 0;
	
	binaryDepthTest(depth - 1, childData[0]);
	binaryDepthTest(depth - 1, childData[1]);

	*data += *childData[0];
	*data += *childData[1];
}