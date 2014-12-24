#pragma once

#include <cstdlib>
#include <cstring>

namespace VGS
{
	namespace Compiler
	{
		struct LitNode
		{
		public:
			char * pText;
			unsigned __int32 size;

		public:
			LitNode(void)
				: pText(nullptr), size(0) {}
			LitNode(char const * const pSource, unsigned __int32 length)
				: pText(nullptr), size(length + 1)
			{				
				if (pText = reinterpret_cast<char*>(malloc(size)))
				{
					memcpy(pText, pSource, length);
					pText[length] = 0;
				}
			}
			~LitNode()
			{
				free(pText);
				pText = nullptr;
			}
		};
	}
}