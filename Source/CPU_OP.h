#pragma once

namespace VGS
{
	// Immediate
	struct CPU_OP_I
	{
		unsigned __int32 op : 6;	// Opcode
		unsigned __int32 rs : 5;	// Source
		unsigned __int32 rt : 5;	// Target
		unsigned __int32 immediate : 16;
	};

	// Jump
	struct CPU_OP_J
	{
		unsigned __int32 op : 6;		// Opcode
		unsigned __int32 target : 26;	// 26 bit address
	};

	// Register
	struct CPU_OP_R
	{
		unsigned __int32 op : 6;
		unsigned __int32 rs : 5;
		unsigned __int32 rt : 5;
		unsigned __int32 rd : 5;
		unsigned __int32 shamt : 5;
		unsigned __int32 funct : 6;
	};
}

///////////////////////////////////////////////////////
// Secondary Intruction
///////////////////////////////////////////////////////

#define CPU_FUNC	0x00	// 0b000000	// Functional Intruction for CPU
#define CPU_BRANCH	0x01	// 0b000001
#define CPU_COP0	0x10	// 0b010000	// Coprocessor 0 (System)
#define CPU_COP1	0x11	// 0b010001	// Coprocessor 1 (Floating Point Unit)

///////////////////////////////////////////////////////
// Load and Store Instructions (All "Immediate")
///////////////////////////////////////////////////////

#define CPU_LB		0x20	// 0b100000	// Load Byte
#define CPU_LBU		0x24	// 0b100100	// Load Byte Unsigned
#define CPU_LH		0x21	// 0b100001	// Load Halfword
#define CPU_LHU		0x25	// 0b100101	// Load Halfword Unsigned
#define CPU_LW		0x23	// 0b100011	// Load Word

#define CPU_SB		0x28	// 0b101000	// Store Byte
#define CPU_SH		0x29	// 0b101001	// Store Halfword
#define CPU_SW		0x2B	// 0b101011	// Store Word

///////////////////////////////////////////////////////
// Arithmetic Instructions
///////////////////////////////////////////////////////

#define CPU_FUNC_ADD	0x20	// 0b100000
#define CPU_ADDI		0x08	// 0b001000
#define CPU_ADDIU		0x09	// 0b001001
#define CPU_FUNC_ADDU	0x21	// 0b100001

#define CPU_FUNC_DIV	0x1A	// 0b011010
#define	CPU_FUNC_DIVU	0x1B	// 0b011011

#define CPU_FUNC_MULT	0x18	// 0b011000
#define CPU_FUNC_MULTU	0x19	// 0b011001

#define CPU_FUNC_SUB	0x22	// 0b100010
#define CPU_FUNC_SUBU	0x23	// 0b100011

///////////////////////////////////////////////////////
// Bitwise Instructions
///////////////////////////////////////////////////////

#define CPU_FUNC_AND	0x24	// 0b100100
#define CPU_ANDI		0x0C	// 0b001100

#define CPU_FUNC_NOR	0x27	// 0b100111

#define CPU_FUNC_OR		0x25	// 0b100101
#define CPU_ORI			0x0D	// 0b001101

#define CPU_FUNC_XOR	0x26	// 0b100110
#define CPU_XORI		0x0E	// 0b001110

///////////////////////////////////////////////////////
// Shifts Instructions
///////////////////////////////////////////////////////

#define CPU_FUNC_SLL	0x00	// 0b000000			
#define CPU_FUNC_SLLV	0x08	// 0b000100

#define CPU_FUNC_SRA	0x03	// 0b000011
#define CPU_SRL			0x02	// 0b000010
#define CPU_SRLV		0x06	// 0b000110

///////////////////////////////////////////////////////
// Comparison Instructions
///////////////////////////////////////////////////////

#define CPU_FUNC_SLT	0x2A	// 0b101010
#define CPU_SLTI		0x0A	// 0b001010
#define CPU_FUNC_SLTU	0x2B	// 0b101011
#define CPU_SLTIU		0x0B	// 0b001011

///////////////////////////////////////////////////////
// Branch Instructions
///////////////////////////////////////////////////////

#define CPU_BEQ		0x04	// 0b000100
#define CPU_BNE		0x05	// 0b000101

#define CPU_BLEZ	0x06	// 0b000110
#define CPU_BGTZ	0x07	// 0b000111

#define CPU_BRANCH_BGEZ		0x01	// 0b00001
#define CPU_BRANCH_BGEZAL	0x11	// 0b10001

#define CPU_BRANCH_BLTZ		0x00	// 0b00000
#define CPU_BRANCH_BLTZAL	0x10	// 0b10000

///////////////////////////////////////////////////////
// Jump Instructions
///////////////////////////////////////////////////////

#define CPU_J			0x02	// 0b000010
#define CPU_JAL			0x03	// 0b000011
#define CPU_JR			0x08	// 0b001000
#define CPU_FUNC_JALR	0x09	// 0b001001

///////////////////////////////////////////////////////
// HI - LO Register Instructions
///////////////////////////////////////////////////////

#define CPU_FUNC_MFHI	0x10	// 0b010000
#define CPU_FUNC_MTHI	0x11	// 0b010001

#define CPU_FUNC_MFLO	0x12	// 0b010010
#define CPU_FUNC_MTLO	0x13	// 0b010011

///////////////////////////////////////////////////////
// Special Instructions
///////////////////////////////////////////////////////

#define CPU_FUNC_BREAK		0x0D	// 0b001101
#define CPU_FUNC_SYSCALL	0x0C	// 0b001100


#define CPU_TRAP	0x1A	// 0b011010
