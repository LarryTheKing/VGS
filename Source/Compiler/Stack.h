#pragma once

#include <cstdlib>
#include <cstring>

namespace VGS
{
	namespace Compiler
	{
		class StackMeasure
		{
		private:
			size_t cursor;

		public:
			StackMeasure(void)
				: cursor(0) {}

			void Clear(void) { cursor = 0; }
			void Align(size_t align)
			{
				cursor = ((cursor + align - 1) / align) * align;
			}

			size_t Alloc(size_t size)
			{
				size_t mark = cursor;
				cursor += size;
				return mark;
			}
			template<class T> T* Alloc(void) { return reinterpret_cast<T*>(Alloc(sizeof(T))); }
			template<class T> T* Alloc(size_t count) { return reinterpret_cast<T*>(Alloc(sizeof(T) * count)); }

			size_t Bytes(void) const { return cursor; }
		};

		class StackAlloc
		{
		private:
			char* begin;
			char* end;
			char* cursor;

		public:
			StackAlloc(void* begin, void* end)
				: begin(reinterpret_cast<char*>(begin)), end(reinterpret_cast<char*>(end)), cursor(reinterpret_cast<char*>(begin)) {}
			StackAlloc(void* begin, size_t size)
				: begin(reinterpret_cast<char*>(begin)), end(reinterpret_cast<char*>(end)+size), cursor(reinterpret_cast<char*>(begin)) {}



			char* Alloc(size_t required, size_t reserve);
			char* Alloc(size_t size) { return Alloc(size, size); }
			template<class T> T* Alloc(void) { return reinterpret_cast<T*>(Alloc(sizeof(T))); }
			template<class T> T* Alloc(size_t count) { return reinterpret_cast<T*>(Alloc(sizeof(T) * count)); }

			bool	Align(size_t align);
			size_t	Capacity() const { return end - begin; }
			void	Reset(void) { cursor = begin; }
			void	Unwind(void const * p);

			char * const Begin(void) const { return begin; }
			char * const Cursor(void) const { return cursor; }
			char * const End(){ return end; }
		};

		class DynamicStackAlloc
		{
		private:
			char* begin;
			char* end;
			char* cursor;

		public:
			DynamicStackAlloc(size_t size);
			~DynamicStackAlloc(void);

			char* Alloc(size_t required, size_t reserve);
			char* Alloc(size_t size) { return Alloc(size, size); }
			template<class T> T* Alloc(void) { return reinterpret_cast<T*>(Alloc(sizeof(T))); }
			template<class T> T* Alloc(size_t count) { return reinterpret_cast<T*>(Alloc(sizeof(T) * count)); }

			bool	Align(size_t align);
			size_t	Capacity() const { return end - begin; }
			void	Reset(void)		{ cursor = begin; }			// Resets the cursor
			bool	Resize(size_t size);
			void	Unwind(void const * p);

			char * const	Begin() const { return begin; }
			char * const	Cursor() const { return cursor; }
			char * const	End() const { return end; }
			size_t	const	Offset(void) const { return cursor - begin; }

		public:
			void Inherit(DynamicStackAlloc * const other)
			{
				begin = other->begin;
				cursor = other->cursor;
				end = other->end;

				other->begin = nullptr;
				other->cursor = nullptr;
				other->end = nullptr;
			}
			void _setBegin(char * ptr) { begin = ptr; }
			void _setCursor(char * ptr) { cursor = ptr; }
			void _setEnd(char * ptr) { end = ptr; }
		};

	}

}

// Parts used from  http://code.google.com/p/eight/
//Copyright (c) 2007 Maciej Sinilo
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
//all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//THE SOFTWARE.