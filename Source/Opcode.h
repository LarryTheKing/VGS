#pragma once

// Immediate
struct OPI
{
	unsigned __int32 op : 6;	// Opcode
	unsigned __int32 rs : 5;	// Source
	unsigned __int32 rt : 5;	// Target
	unsigned __int32 immediate : 16;
};

// Jump
struct OPJ
{
	unsigned __int32 op : 6;	// Opcode
	unsigned __int32 target : 26	// 26 bit address
};

// Register
struct OPR
{
	unsigned __int32 op : 6;
	unsigned __int32 rs : 5;
	unsigned __int32 rt : 5;
	unsigned __int32 rd : 5;
	unsigned __int32 shamt : 5;
	unsigned __int32 funct : 6;
};

#define NOP	0x00

#define ADD 0x00	//