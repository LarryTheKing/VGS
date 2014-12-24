#include "CPU.h"
#include "System.h"
#include "CPU_OP.h"

#define ADVANCE_PC(a )	PC.u += a

namespace VGS
{
	CPU::CPU(System * const pS) :	pSystem(pS) 
	{
		// Zero registers
		for (int i = 0; i < 0x20; i++)
			reg[i].u = 0u;

		// Set SP and FP to the address of last 4 bytes of RAM
		SP.u = FP.u= pS->vRAM.size - 4;
	}

	struct CPU_OP
	{
		union
		{
			unsigned __int32 WORD;
			CPU_OP_I	I;
			CPU_OP_J	J;
			CPU_OP_R	R;
		};
	};

	bool CPU::Cycle()
	{
		CPU_OP OP;
		OP.WORD = pSystem->GetMem<unsigned __int32>(PC.u);

		switch (OP.I.op)
		{
	#pragma region FUNCTIONAL
		case CPU_FUNC:
		{
			switch (OP.R.funct)
			{
			case CPU_FUNC_ADD :
				reg[OP.R.rd].i = reg[OP.R.rs].i + reg[OP.R.rt].i;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_ADDU :
				reg[OP.R.rd].u = reg[OP.R.rs].u + reg[OP.R.rt].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_AND :
				reg[OP.R.rd].u = reg[OP.R.rs].u & reg[OP.R.rt].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_BREAK :
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_DIV :
				LO.i = reg[OP.R.rs].i / reg[OP.R.rt].i;
				HI.i = reg[OP.R.rs].i % reg[OP.R.rt].i;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_DIVU :
				LO.u = reg[OP.R.rs].u / reg[OP.R.rt].u;
				HI.u = reg[OP.R.rs].u % reg[OP.R.rt].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_JALR :
				reg[OP.R.rd].u = PC.u + 4;
				PC.u = (MS_MASK & PC.u) | reg[OP.R.rs].u;
				break;
			case CPU_FUNC_JR :
				PC.u = reg[OP.R.rs].u;
				break;
			case CPU_FUNC_MFHI :
				reg[OP.R.rd].u = HI.u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_MFLO :
				reg[OP.R.rd].u = LO.u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_MTHI :
				HI.u = reg[OP.R.rs].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_MTLO :
				LO.u = reg[OP.R.rs].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_MULT :
				LO.i = reg[OP.R.rs].i * reg[OP.R.rt].i;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_MULTU :
				LO.u = reg[OP.R.rs].u * reg[OP.R.rt].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_NOR :
				reg[OP.R.rd].u = ~(reg[OP.R.rs].u | reg[OP.R.rt].u);
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_OR :
				reg[OP.R.rd].u = reg[OP.R.rs].u | reg[OP.R.rt].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SLL :
				reg[OP.R.rd].u = reg[OP.R.rt].u << OP.R.shamt;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SLLV :
				reg[OP.R.rd].u = reg[OP.R.rt].u << reg[OP.R.rs].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SLT :
				reg[OP.R.rd].u = (reg[OP.I.rs].i < reg[OP.I.rt].i ? 1 : 0);
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SLTU :
				reg[OP.R.rd].u = (reg[OP.I.rs].u < reg[OP.I.rt].u ? 1 : 0);
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SRA :
				reg[OP.R.rd].u = (reg[OP.R.rt].u >> OP.R.shamt) | (~(0xFFFFFFFF >> OP.R.shamt));
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SRL :
				reg[OP.R.rd].u = reg[OP.R.rt].u >> OP.R.shamt;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SRLV :
				reg[OP.R.rd].u = reg[OP.R.rt].u >> reg[OP.R.rs].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SUB :
				reg[OP.R.rd].i = reg[OP.R.rs].i - reg[OP.R.rt].i;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SUBU :
				reg[OP.R.rd].u = reg[OP.R.rs].u - reg[OP.R.rt].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_XOR :
				reg[OP.R.rd].u = reg[OP.R.rs].u ^ reg[OP.R.rt].u;
				ADVANCE_PC(4);
				break;
			case CPU_FUNC_SYSCALL :
				pSystem->SystemCall(V[0].u);
				ADVANCE_PC(4);
				break;
			}
		} break;
	#pragma endregion
	#pragma region IMMEDIATE_JUMP
		case CPU_ADDI :
			reg[OP.I.rt].i = reg[OP.I.rs].i + OP.I.i;
			ADVANCE_PC(4);
			break;
		case CPU_ADDIU :
			reg[OP.I.rt].u = reg[OP.I.rs].u + OP.I.i;
			ADVANCE_PC(4);
			break;
		case CPU_ANDI :
			reg[OP.I.rt].u = reg[OP.I.rs].u & OP.I.u;
			ADVANCE_PC(4);
			break;
		case CPU_BEQ :
			ADVANCE_PC((reg[OP.I.rs].u == reg[OP.I.rt].u ? OP.I.u << 2 : 4));
			break;
		case CPU_BGTZ :
			ADVANCE_PC((reg[OP.I.rs].u > 0 ? OP.I.u << 2 : 4));
			break;
		case CPU_BLEZ:
			ADVANCE_PC((reg[OP.I.rs].u <= 0 ? OP.I.u << 2 : 4));
			break;
		case CPU_BNE:
			ADVANCE_PC((reg[OP.I.rs].u != reg[OP.I.rt].u ? OP.I.u << 2 : 4));
			break;
		case CPU_JAL:
			RA.u = PC.u + 4;
		case CPU_J:
			PC.u = (MS_MASK & PC.u) | ( OP.J.target << 2 );
			break;
		case CPU_LB :
			reg[OP.I.rt].i = pSystem->GetMem<__int8>(reg[OP.I.rs].u + OP.I.i);
			ADVANCE_PC(4);
			break;
		case CPU_LBU:
			reg[OP.I.rt].u = pSystem->GetMem<unsigned __int8>(reg[OP.I.rs].u + OP.I.i);
			ADVANCE_PC(4);
			break;
		case CPU_LH :
			reg[OP.I.rt].i = pSystem->GetMem<__int16>(reg[OP.I.rs].u + OP.I.i);
			ADVANCE_PC(4);
			break;
		case CPU_LHU:
			reg[OP.I.rt].u = pSystem->GetMem<unsigned __int16>(reg[OP.I.rs].u + OP.I.i);
			ADVANCE_PC(4);
			break;
		case CPU_LW :
			reg[OP.I.rt].i = pSystem->GetMem<__int32>(reg[OP.I.rs].u + OP.I.i);
			ADVANCE_PC(4);
			break;
		case CPU_ORI :
			reg[OP.I.rt].u = reg[OP.I.rs].u | OP.I.u;
			ADVANCE_PC(4);
			break;
		case CPU_SB :
			pSystem->SetMem<unsigned __int8>(reg[OP.I.rs].u + OP.I.i, 0xFF & reg[OP.I.rt].u);
			ADVANCE_PC(4);
			break;
		case CPU_SH :
			pSystem->SetMem<unsigned __int16>(reg[OP.I.rs].u + OP.I.i, 0xFFFF & reg[OP.I.rt].u);
			ADVANCE_PC(4);
			break;
		case CPU_SLTI :
			reg[OP.I.rt].u = (reg[OP.I.rs].i < OP.I.i ? 1 : 0);
			ADVANCE_PC(4);
			break;
		case CPU_SLTIU :
			reg[OP.I.rt].u = (reg[OP.I.rs].u < OP.I.u ? 1 : 0);
			ADVANCE_PC(4);
		case CPU_SW :
			pSystem->SetMem<unsigned __int32>(reg[OP.I.rs].u + OP.I.i, reg[OP.I.rt].u);
			ADVANCE_PC(4);
			break;
		case CPU_XORI :
			reg[OP.I.rt].u = reg[OP.I.rs].u ^ OP.I.u;
			ADVANCE_PC(4);
			break;
	#pragma endregion
		default:
			ADVANCE_PC(4);
			break;
		}

		// Register 0 must always equal zero
		reg[0].u = 0;

		return true;
	}
}