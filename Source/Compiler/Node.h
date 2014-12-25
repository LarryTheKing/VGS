#pragma once

#include <string>

#define NODE_TYPE_NONE		0x00
#define NODE_TYPE_REGISTER	0x01
#define NODE_TYPE_ADDRESS	0x02
#define NODE_TYPE_OP		0xF0
#define NODE_TYPE_RS		0xF1
#define NODE_TYPE_RT		0xF2
#define NODE_TYPE_IMMEDIATE	0xF3
#define NODE_TYPE_OFFSET	0xF4
#define NODE_TYPE_TARGET	0xF5
#define NODE_TYPE_RD		0xF6
#define NODE_TYPE_SHAMT		0xF7
#define NODE_TYPE_FUNCT		0xF8

namespace VGS
{
	namespace Compiler
	{
		struct ProcNode
		{
			std::string			ID;
			unsigned __int32	Value;
			union
			{
				unsigned __int32	Type;
				struct
				{
					unsigned __int8 t8;
					unsigned __int8 p1;
					unsigned __int8 p2;
					unsigned __int8 p3;
				};
			};
			
		};
	}
}