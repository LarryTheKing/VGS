#include "Stack.h"
#include <cassert>

namespace VGS
{
	namespace Compiler
	{
		bool StackAlloc::Align(size_t align)
		{
			char* null = (char*)0;
			ptrdiff_t offset = cursor - null;
			offset = ((offset + align - 1) / align) * align;
			char* newCursor = null + offset;
			if (newCursor <= end)
			{
				cursor = newCursor;
				return  true;
			}
			return false;
		}

		char* StackAlloc::Alloc(size_t required, size_t reserve)
		{
			assert(reserve >= required);
			if (cursor + reserve <= end)
			{
				char* allocation = cursor;
				cursor += required;
				assert(allocation);
				memset(allocation, 0xCDCDCD, required);
			}
			return nullptr;
		}

		void StackAlloc::Unwind(void const * p)
		{
			assert(p >= begin && p < end && p <= cursor);
			size_t bytes = cursor - (char*)p;
			cursor = (char*)p;
			memset(cursor, 0xFEFEFE, bytes);
		}

		DynamicStackAlloc::DynamicStackAlloc(size_t size)
		{
			begin = cursor = (char*)malloc(size);
			end = begin + size;
		}
		DynamicStackAlloc::~DynamicStackAlloc()
		{
			if (begin != nullptr)
			{
				free(begin);
				begin = nullptr;
				end = nullptr;
				cursor = nullptr;
			}
		}

		bool DynamicStackAlloc::Align(size_t align)
		{
			// Avert bad case scenarios
			if (cursor + align > end)
			{
				if (!Resize(2 * Capacity()))
					return false;
			}

			char* null = (char*)0;
			ptrdiff_t offset = cursor - null;
			offset = ((offset + align - 1) / align) * align;
			char* newCursor = null + offset;
			if (newCursor <= end)
			{
				cursor = newCursor;
				return  true;
			}
			return false;
		}

		bool DynamicStackAlloc::Resize(size_t size)
		{
			void* p = realloc(begin, size);
			if (p != nullptr)
			{
				if (p != begin)
				{
					// free(begin);
					cursor = cursor - begin + (char*)p;
				}

				begin = (char*)p;
				end = begin + size;
				return true;
			}
			return false;
		}

		char* DynamicStackAlloc::Alloc(size_t required, size_t reserve)
		{
			assert(reserve >= required);

			if (cursor + reserve > end)
			{
				// Attempt to resize stack
				assert(Resize(2 * Capacity()));
			}

			if (cursor + reserve <= end)
			{
				char* allocation = cursor;
				cursor += required;
				assert(allocation);
				memset(allocation, 0xCDCDCD, required);
				return allocation;
			}
			return nullptr;
		}

		void DynamicStackAlloc::Unwind(void const * p)
		{
			assert(p >= begin && p < end && p <= cursor);
			size_t bytes = cursor - (char*)p;
			cursor = (char*)p;
			memset(cursor, 0xFEFEFE, bytes);
		}
	}
}