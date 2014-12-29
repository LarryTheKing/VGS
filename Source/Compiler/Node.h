#pragma once

#include <string>

#define NODE_TYPE_NONE		0x00
#define NODE_TYPE_REGISTER	0x01
#define NODE_TYPE_ADDRESS	0x02
#define NODE_TYPE_OFFSET_T	0x10	// Text : ROM
#define NODE_TYPE_OFFSET_D	0x11	// Data : RAM
#define NODE_TYPE_OFFSET_R	0x12	// Read only data : ROM
#define NODE_TYPE_OFFSET_B	0x13	// BSS
#define NODE_TYPE_REGION	0x20
#define NODE_TYPE_LOWER		0x30
#define NODE_TYPE_UPPER		0x31
#define NODE_TYPE_OP		0xF0
#define NODE_TYPE_RS		0xF1
#define NODE_TYPE_RT		0xF2
#define NODE_TYPE_IMMEDIATE	0xF3
#define NODE_TYPE_OFFSET	0xF4
#define NODE_TYPE_TARGET	0xF5
#define NODE_TYPE_RD		0xF6
#define NODE_TYPE_SHAMT		0xF7
#define NODE_TYPE_FUNCT		0xF8
#define NODE_TYPE_RTARGET	0xF9
#define NODE_TYPE_OP_S		0xFF	// Special ops, ie la	

namespace VGS
{
	namespace Compiler
	{
		struct iNode
		{
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

		struct ProcNode
		{
			std::string		ID;
			iNode			i;

			ProcNode(void){}
			ProcNode(std::string ID, unsigned __int32 Value, unsigned __int32 Type)
				:ID(ID)
			{
				i.Value = Value;
				i.Type = Type;
			}
		};

		struct LinkNode
		{
			std::string ID;				//
			unsigned __int32 Offset;	// Offset of opcode that needs updating
			unsigned __int32 Type;		// 
		};
	}
}