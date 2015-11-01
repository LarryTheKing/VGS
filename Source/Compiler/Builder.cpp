#include "Builder.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <initializer_list>



namespace VGS
{
	namespace Compiler
	{

		Builder::Builder(void)
			: s_text(128), s_data(128), s_rodata(128), s_bss(0), s_strtab(128), s_symtab(128), s_rela(128)
		{
			LoadLanguage();
		}

		Builder::~Builder()
		{
			Reset();
		}

		void Builder::Reset(void)
		{
			Source.clear();
			s_text.Reset();
			s_data.Reset();
			s_rodata.Reset();
			s_bss = 0;
			s_strtab.Reset(); *s_strtab.Alloc<char>() = 0;	// First byte is always '\0'
			s_symtab.Reset();
			s_rela.Reset();
			Labels.clear();
			References.clear();
		}

		bool Builder::Build(char const * const fin, char const * const fout)
		{
			std::cout << std::endl << "Building file : \"" << fin << "\"" << std::endl;

			Reset();
			if (!GenSource(fin))
			{
				std::cout << "ERROR : Failed to create source tree" << std::endl;
				return false;
			}

			unsigned __int32 retGPT = GenProgTree();
			if (retGPT != _UI32_MAX)
			{
				std::cout << "ERROR : Failed to create program tree" << std::endl;
				std::cout << ">> Line " << Source[retGPT].Line << " : \"" << Source[retGPT].Text << "\"" <<std::endl <<std::endl;
				return false;
			}

			if (!GenELF(fout))
			{
				std::cout << "ERROR : Failed to create ELF object" << std::endl;
				return false;
			}

			std::cout << std::endl << "Build successful!" << std::endl;

			return true;
		}

#define FUNC_OP(op)	(op << 0x1A)

#define NODE_TYPE_RS		0xF1
#define NODE_TYPE_RT		0xF2
#define NODE_TYPE_IMMEDIATE	0xF3
#define NODE_TYPE_OFFSET	0xF4
#define NODE_TYPE_TARGET	0xF5
#define NODE_TYPE_RD		0xF6
#define NODE_TYPE_SHAMT		0xF7
#define NODE_TYPE_FUNCT		0xF8
#define NODE_TYPE_RTARGET	0xF9

#define SOP_CODE_LA		0x01
#define SOP_CODE_LI		0x02
#define SOP_CODE_PUSH	0x03
#define SOP_CODE_POP	0x04
		void Builder::LoadLanguage(void)
		{
			Language.push_back(Semantic("UNDEFINED"));
#pragma region REGISTERS
			Language.push_back(Semantic("$r0",	SEMANTIC_TYPE_REG, { 0x00 }));
			Language.push_back(Semantic("$r1",	SEMANTIC_TYPE_REG, { 0x01 }));
			Language.push_back(Semantic("$r2",	SEMANTIC_TYPE_REG, { 0x02 }));
			Language.push_back(Semantic("$r3",	SEMANTIC_TYPE_REG, { 0x03 }));
			Language.push_back(Semantic("$r4",	SEMANTIC_TYPE_REG, { 0x04 }));
			Language.push_back(Semantic("$r5",	SEMANTIC_TYPE_REG, { 0x05 }));
			Language.push_back(Semantic("$r6",	SEMANTIC_TYPE_REG, { 0x06 }));
			Language.push_back(Semantic("$r7",	SEMANTIC_TYPE_REG, { 0x07 }));
			Language.push_back(Semantic("$r8",	SEMANTIC_TYPE_REG, { 0x08 }));
			Language.push_back(Semantic("$r9",	SEMANTIC_TYPE_REG, { 0x09 }));
			Language.push_back(Semantic("$r10",	SEMANTIC_TYPE_REG, { 0x0A }));
			Language.push_back(Semantic("$r11", SEMANTIC_TYPE_REG, { 0x0B }));
			Language.push_back(Semantic("$r12", SEMANTIC_TYPE_REG, { 0x0C }));
			Language.push_back(Semantic("$r13", SEMANTIC_TYPE_REG, { 0x0D }));
			Language.push_back(Semantic("$r14", SEMANTIC_TYPE_REG, { 0x0E }));
			Language.push_back(Semantic("$r15", SEMANTIC_TYPE_REG, { 0x0F }));
			Language.push_back(Semantic("$r16", SEMANTIC_TYPE_REG, { 0x10 }));
			Language.push_back(Semantic("$r17", SEMANTIC_TYPE_REG, { 0x11 }));
			Language.push_back(Semantic("$r18", SEMANTIC_TYPE_REG, { 0x12 }));
			Language.push_back(Semantic("$r19", SEMANTIC_TYPE_REG, { 0x13 }));
			Language.push_back(Semantic("$r20", SEMANTIC_TYPE_REG, { 0x14 }));
			Language.push_back(Semantic("$r21", SEMANTIC_TYPE_REG, { 0x15 }));
			Language.push_back(Semantic("$r22", SEMANTIC_TYPE_REG, { 0x16 }));
			Language.push_back(Semantic("$r23", SEMANTIC_TYPE_REG, { 0x17 }));
			Language.push_back(Semantic("$r24", SEMANTIC_TYPE_REG, { 0x18 }));
			Language.push_back(Semantic("$r25", SEMANTIC_TYPE_REG, { 0x19 }));
			Language.push_back(Semantic("$r26", SEMANTIC_TYPE_REG, { 0x1A }));
			Language.push_back(Semantic("$r27", SEMANTIC_TYPE_REG, { 0x1B }));
			Language.push_back(Semantic("$r28", SEMANTIC_TYPE_REG, { 0x1C }));
			Language.push_back(Semantic("$r29", SEMANTIC_TYPE_REG, { 0x1D }));
			Language.push_back(Semantic("$r30", SEMANTIC_TYPE_REG, { 0x1E }));
			Language.push_back(Semantic("$r31", SEMANTIC_TYPE_REG, { 0x1F }));

			Language.push_back(Semantic("$v0", SEMANTIC_TYPE_REG, { 0x02 }));
			Language.push_back(Semantic("$v1", SEMANTIC_TYPE_REG, { 0x03 }));

			Language.push_back(Semantic("$a0", SEMANTIC_TYPE_REG, { 0x04 }));
			Language.push_back(Semantic("$a1", SEMANTIC_TYPE_REG, { 0x05 }));
			Language.push_back(Semantic("$a2", SEMANTIC_TYPE_REG, { 0x06 }));
			Language.push_back(Semantic("$a3", SEMANTIC_TYPE_REG, { 0x07 }));

			Language.push_back(Semantic("$t0", SEMANTIC_TYPE_REG, { 0x08 }));
			Language.push_back(Semantic("$t1", SEMANTIC_TYPE_REG, { 0x09 }));
			Language.push_back(Semantic("$t2", SEMANTIC_TYPE_REG, { 0x0A }));
			Language.push_back(Semantic("$t3", SEMANTIC_TYPE_REG, { 0x0B }));
			Language.push_back(Semantic("$t4", SEMANTIC_TYPE_REG, { 0x0C }));
			Language.push_back(Semantic("$t5", SEMANTIC_TYPE_REG, { 0x0D }));
			Language.push_back(Semantic("$t6", SEMANTIC_TYPE_REG, { 0x0E }));
			Language.push_back(Semantic("$t7", SEMANTIC_TYPE_REG, { 0x0F }));

			Language.push_back(Semantic("$s0", SEMANTIC_TYPE_REG, { 0x10 }));
			Language.push_back(Semantic("$s1", SEMANTIC_TYPE_REG, { 0x11 }));
			Language.push_back(Semantic("$s2", SEMANTIC_TYPE_REG, { 0x12 }));
			Language.push_back(Semantic("$s3", SEMANTIC_TYPE_REG, { 0x13 }));
			Language.push_back(Semantic("$s4", SEMANTIC_TYPE_REG, { 0x14 }));
			Language.push_back(Semantic("$s5", SEMANTIC_TYPE_REG, { 0x15 }));
			Language.push_back(Semantic("$s6", SEMANTIC_TYPE_REG, { 0x16 }));
			Language.push_back(Semantic("$s7", SEMANTIC_TYPE_REG, { 0x17 }));

			Language.push_back(Semantic("$t8", SEMANTIC_TYPE_REG, { 0x18 }));
			Language.push_back(Semantic("$t9", SEMANTIC_TYPE_REG, { 0x19 }));
			Language.push_back(Semantic("$k0", SEMANTIC_TYPE_REG, { 0x1A }));
			Language.push_back(Semantic("$k1", SEMANTIC_TYPE_REG, { 0x1B }));
			Language.push_back(Semantic("$gp", SEMANTIC_TYPE_REG, { 0x1C }));
			Language.push_back(Semantic("$sp", SEMANTIC_TYPE_REG, { 0x1D }));
			Language.push_back(Semantic("$fp", SEMANTIC_TYPE_REG, { 0x1E }));
			Language.push_back(Semantic("$ra", SEMANTIC_TYPE_REG, { 0x1F }));
#pragma endregion
#pragma region OPS
			Language.push_back(Semantic("lb",	SEMANTIC_TYPE_OP, { CPU_LB,		NODE_TYPE_RT, NODE_TYPE_OFFSET }));
			Language.push_back(Semantic("lh",	SEMANTIC_TYPE_OP, { CPU_LH,		NODE_TYPE_RT, NODE_TYPE_OFFSET }));
			Language.push_back(Semantic("lw",	SEMANTIC_TYPE_OP, { CPU_LW,		NODE_TYPE_RT, NODE_TYPE_OFFSET }));
			Language.push_back(Semantic("lbu",	SEMANTIC_TYPE_OP, { CPU_LBU,	NODE_TYPE_RT, NODE_TYPE_OFFSET }));
			Language.push_back(Semantic("lhu",	SEMANTIC_TYPE_OP, { CPU_LHU,	NODE_TYPE_RT, NODE_TYPE_OFFSET }));
			Language.push_back(Semantic("lui",	SEMANTIC_TYPE_OP, { CPU_LUI,	NODE_TYPE_RT, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("sb",	SEMANTIC_TYPE_OP, { CPU_SB,		NODE_TYPE_RT, NODE_TYPE_OFFSET }));
			Language.push_back(Semantic("sh",	SEMANTIC_TYPE_OP, { CPU_SH,		NODE_TYPE_RT, NODE_TYPE_OFFSET }));
			Language.push_back(Semantic("sw",	SEMANTIC_TYPE_OP, { CPU_SW,		NODE_TYPE_RT, NODE_TYPE_OFFSET }));

			Language.push_back(Semantic("addi", SEMANTIC_TYPE_OP, { CPU_ADDI,	NODE_TYPE_RT, NODE_TYPE_RS, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("addiu",SEMANTIC_TYPE_OP, { CPU_ADDIU,	NODE_TYPE_RT, NODE_TYPE_RS, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("addi", SEMANTIC_TYPE_OP, { CPU_ADDI, NODE_TYPE_RT, NODE_TYPE_RS, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("add",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_ADD),	NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("addu", SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_ADDU), NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("sub",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_SUB),	NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("subu", SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_SUBU), NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("mult",		SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_MULT),		NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("multu",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_MULTU),	NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("div",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_DIV),	NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("divu",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_DIVU),	NODE_TYPE_RS, NODE_TYPE_RT }));

			Language.push_back(Semantic("andi", SEMANTIC_TYPE_OP, { CPU_ANDI,	NODE_TYPE_RT, NODE_TYPE_RS, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("ori",	SEMANTIC_TYPE_OP, { CPU_ORI,	NODE_TYPE_RT, NODE_TYPE_RS, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("xori", SEMANTIC_TYPE_OP, { CPU_XORI,	NODE_TYPE_RT, NODE_TYPE_RS, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("and",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_AND),	NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("nor",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_NOR),	NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("or",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_OR),	NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("xor",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_XOR),	NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));

			Language.push_back(Semantic("slti",		SEMANTIC_TYPE_OP, { CPU_SLTI,	NODE_TYPE_RT, NODE_TYPE_RS, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("sltiu",	SEMANTIC_TYPE_OP, { CPU_SLTIU,	NODE_TYPE_RT, NODE_TYPE_RS, NODE_TYPE_IMMEDIATE }));
			Language.push_back(Semantic("slt",		SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_SLT),	NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));
			Language.push_back(Semantic("sltu",		SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_SLTU), NODE_TYPE_RD, NODE_TYPE_RS, NODE_TYPE_RT }));

			Language.push_back(Semantic("beq",	SEMANTIC_TYPE_OP, { CPU_BEQ,	NODE_TYPE_RS, NODE_TYPE_RT, NODE_TYPE_RTARGET }));
			Language.push_back(Semantic("bne",	SEMANTIC_TYPE_OP, { CPU_BNE,	NODE_TYPE_RS, NODE_TYPE_RT, NODE_TYPE_RTARGET }));
			Language.push_back(Semantic("blez", SEMANTIC_TYPE_OP, { CPU_BLEZ,	NODE_TYPE_RS, NODE_TYPE_RTARGET }));
			Language.push_back(Semantic("bgtz", SEMANTIC_TYPE_OP, { CPU_BGTZ,	NODE_TYPE_RS, NODE_TYPE_RTARGET }));
			Language.push_back(Semantic("bgez",		SEMANTIC_TYPE_OP, { 0x0801, NODE_TYPE_RS, NODE_TYPE_RTARGET }));
			Language.push_back(Semantic("bgezal",	SEMANTIC_TYPE_OP, { 0x8801, NODE_TYPE_RS, NODE_TYPE_RTARGET }));
			Language.push_back(Semantic("bltz",		SEMANTIC_TYPE_OP, { 0x0001, NODE_TYPE_RS, NODE_TYPE_RTARGET }));
			Language.push_back(Semantic("bltzal",	SEMANTIC_TYPE_OP, { 0x8001, NODE_TYPE_RS, NODE_TYPE_RTARGET }));

			Language.push_back(Semantic("j",	SEMANTIC_TYPE_OP, { CPU_J,		NODE_TYPE_TARGET }));
			Language.push_back(Semantic("jal",	SEMANTIC_TYPE_OP, { CPU_JAL,	NODE_TYPE_TARGET }));
			Language.push_back(Semantic("jr",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_JR),	NODE_TYPE_RS}));
			Language.push_back(Semantic("jalr",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_JALR), NODE_TYPE_RS, NODE_TYPE_RD }));

			Language.push_back(Semantic("mfhi", SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_MFHI), NODE_TYPE_RD }));
			Language.push_back(Semantic("mflo", SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_MFLO), NODE_TYPE_RD }));
			Language.push_back(Semantic("mthi", SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_MTHI), NODE_TYPE_RS }));
			Language.push_back(Semantic("mtlo", SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_MTLO), NODE_TYPE_RS }));

			Language.push_back(Semantic("break",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_BREAK)}));
			Language.push_back(Semantic("syscall",	SEMANTIC_TYPE_OP, { FUNC_OP(CPU_FUNC_SYSCALL)}));

			Language.push_back(Semantic("nop", SEMANTIC_TYPE_OP, { 0 }));
			Language.push_back(Semantic("mov", SEMANTIC_TYPE_OP, { CPU_ADDI, NODE_TYPE_RT, NODE_TYPE_RS }));

			Language.push_back(Semantic("la",	SEMANTIC_TYPE_OPS, { SOP_CODE_LA }));
			Language.push_back(Semantic("li",	SEMANTIC_TYPE_OPS, { SOP_CODE_LI }));
			Language.push_back(Semantic("push", SEMANTIC_TYPE_OPS, { SOP_CODE_PUSH }));
			Language.push_back(Semantic("pop",	SEMANTIC_TYPE_OPS, { SOP_CODE_POP }));
#pragma endregion
			std::cout << ">> Found " << Language.size() - 1 << " terms in language" << std::endl;
		}
		
		bool Builder::GenSource(char const * const fileName)
		{
			std::ifstream ifile(fileName);
			std::cout << ">> Opening file \"" << fileName << "\"\n";
			if (!ifile.is_open())
			{
				std::cout << "ERROR : Failed to open file." << std::endl;
				return false;
			}

			// Read each line from file and generate source tree

			std::cout << "\nGenerating source tree...\n";

			unsigned __int32 nLine = 1;

			std::string	sLine;
			while (std::getline(ifile, sLine))
			{
				if (!GenSourceNodes(sLine, nLine))
				{
					std::cout << "ERROR : Unable to parse line " << nLine << std::endl;
					return false;
				}
				nLine++;
			}

			std::cout << ">> Found " << Source.size() << " expressions" << std::endl;

			return true;
			// TODO : Allow include files
		}
		
		// Finds expressions in a line of text and adds them to the source
		bool Builder::GenSourceNodes(std::string const sLine, const unsigned __int32 nLine)
		{
			if (!sLine[0])		// Skip empty lines
				return true;

			size_t	index	= 0;		// The index of the start of the keyword
			bool	bQuote	= false;	// If this keyword is a quoute, we will include spaces and other return characters

			// We will parse the line one character at a time looking for individual terms
			// We will keep parsing until we encounter a comment, a new line, or have parsed the entire line
			for (size_t i = 0; i < sLine.size() + 1; i++)	
			{
				// Is this the start or end of a quote?
				// Could this also be a quotation mark within a quote?
				if (sLine[i] == '\"')	
				{
					if (bQuote && sLine[i - 1] != '\\')	{	// If we are alleady in a quote and we encounter another quotation mark, end the quote
						bQuote = false;	}					// unless this is part of the escape sequence for a quotation character
					else
						bQuote = true;						// This is a new quote
				}
				// The current term has come to an end add it to the vocab list
				else if ((!bQuote && (sLine[i] == ' ' || sLine[i] == ',' || sLine[i] == '\t')) || sLine[i] == '\0')
				{
					if (i > index) // Have we read something?
					{
						// Add the term to the expression list
						Source.push_back(Expression(sLine.substr(index, i - index), nLine));
					}

					index = i + 1;

					if (!sLine[i])	// This line is done when we encounter the /0 character
						return true;
				}
				// If this is a comment or the line ends, stop parsing
				else if (sLine[i] == '#' || sLine[i] == '\n')
					return true;
			}

			return false;
		}
		
		bool ValidateName(const std::string & s)
		{
			// A valid name must start with a letter or an underscore
			if (s[0] > 64 && s[0] < 91)	// Is this a capital letter?
				return true;
			else if (s[0] > 96 && s[0] < 123)	// Is this a lowercase letter?
				return true;
			else if (s[0] == 95)	// Is this an underscore?
				return true;

			return false;	// This is none of those things
		}

		bool Builder::AddGlobal(std::string const s)
		{
			// Is this a valid name for a global?
			if (!ValidateName(s))
				return false;

			// Does another global with this name already exist?
			// We don't won't to add two of the same values to the global list
			for (unsigned __int32 i = 0; i < Globals.size(); i++)
			{	
				if (s == Globals[i].ID)
					return true;
			}

			// Add a new Global
			Globals.push_back(Global(s));
			return true;
		}

		bool Builder::AddLabel(Label const n)
		{
			// Check that this label has a valid name
			if (!ValidateName(n.ID))
				return false;

			// See if we have already defined this label
			for (size_t i = 0; i < Labels.size(); i++)
			{
				if (n.ID == Labels[i].ID)
				{
					if (Labels[i].IsDefined())
					{
						std::cout << "ERROR : Label redefinition -" << n.ID << std::endl;
						return false;
					}
				}
			}

			Labels.push_back(n);
			return true;
		}

		Semantic const * Builder::MatchNode(const std::string & rString) const
		{
			const size_t length = Language.size();
			for (size_t i = 1; i < length; i++)
			{
				// Check for match
				if (rString == Language[i].ID)
					return &Language[i];
			}

			// No match found, return UNDEFINED node
			return &Language[0];
		}

#define MODE_TEXT	0x01
#define MODE_DATA	0x02
#define MODE_RODATA	0x03
#define MODE_BSS	0x04

		unsigned __int32 Builder::GenProgTree(void)
		{
			std::cout << "\nGenerating program tree...\n";

			unsigned __int32 mode = MODE_TEXT;	// Assume that we are in .text mode to start

			const size_t length = Source.size();// How many source expressions are there

			// We will evaluate, in order, each source expression
			for (size_t i = 0; i < length; i++)
			{
				// We care about the actual text portion of the expression
				std::string const cWord = Source[i].Text;

				// Are we changing modes?
				if (cWord == ".text")
					mode = MODE_TEXT;
				else if (cWord == ".data")
					mode = MODE_DATA;
				else if (cWord == ".rodata")
					mode = MODE_RODATA;
				else if (cWord == ".bss")
					mode = MODE_BSS;
				else if (cWord == ".global")
				{
					// Try to add the next expression to the global list
					if (!AddGlobal(Source[++i].Text))
						return i;	// If we fail, identify the expression
				}
				else if (mode == MODE_TEXT)
				{
					// Is this a label?
					if (cWord[cWord.length() - 1] == ':')
					{
						// Try to add the label to the label list
						if (!AddLabel(Label(cWord.substr(0, cWord.length() - 1), s_text.Offset(), LABEL_TYPE_BANK_T)))
							return i;
					}
					else
					{
						// Try to add an opcode to the .text section
						unsigned __int32 const s = GenText(&Source[i]);
						if (_UI32_MAX != s)	// Was there an error?
							i += s; // Add the number of expressions we used to the current index
						else
							return i;
					}
				}
				else if (mode == MODE_DATA || mode == MODE_RODATA)
				{
					unsigned __int32 const s = GenData(&Source[i], &Source.back(), mode);
					if (_UI32_MAX != s)
						i += s;
					else
						return i;
				}
				else if (mode == MODE_BSS)
				{
					unsigned __int32 const s = GenBss(&Source[i], &Source.back());
					if (_UI32_MAX != s)
						i += s;
					else
						return i;
				}
			}

			std::cout << ">> Program tree built\n>> Created " << Labels.size() << " labels (" << Globals.size() << " global)\n>> Created " << References.size() << " references\n";

			return _UI32_MAX;
		}

		unsigned __int32 Builder::GenData(Expression const * pArgs, Expression const * const pLast, unsigned __int32 mode)
		{
			DynamicStackAlloc * pStack = nullptr;

			// Identify which memory bank we need to add this data to
			if (mode == MODE_DATA)
				pStack = &s_data;
			else if (mode == MODE_RODATA)
				pStack = &s_rodata;
			else
				return _UI32_MAX;

			int nArgs = 0;

			if (pArgs == pLast)		// Check for last expression
				return _UI32_MAX;

			// Is there a label?
			#pragma region LABEL
			if (pArgs[0].Text[pArgs[0].Text.length() - 1] == ':')
			{
				if (pArgs + 1 == pLast)		// Check for end of litearture
					return _UI32_MAX;

				//  Align to data sizes
				if (pArgs[1].Text == ".half")
				{
					if (!pStack->Align(2))
						return _UI32_MAX;
				}
				else if (pArgs[1].Text == ".word" || pArgs[1].Text == ".space")
				{
					if (!pStack->Align(4))
						return _UI32_MAX;
				}

				// Add label to list
				if (!AddLabel(Label(pArgs[0].Text.substr(0, pArgs[0].Text.length() - 1), pStack->Offset(), (mode == MODE_DATA ? LABEL_TYPE_BANK_D : LABEL_TYPE_BANK_R))))
					return _UI32_MAX;
				
				pArgs++;
				nArgs++;
			}
			#pragma endregion

			if (pArgs->Text == ".byte")
			{
				#pragma region BYTE
				pArgs++;

				while (long long val = strtoll(pArgs->Text.data(), nullptr, 0))
				{
					if (val == 0 && pArgs->Text[0] != '0')
						break;

					if (val < _I8_MIN || val > _UI8_MAX)
					{
						std::cout << "ERROR : Value " << val << " cannot be stored in a byte\n";
						return _UI32_MAX;
					}

					if (val < 0)
						*pStack->Alloc<__int8>() = static_cast<__int8>(val);
					else
						*pStack->Alloc<unsigned __int8>() = static_cast<unsigned __int8>(val);

					nArgs++;

					if (pArgs != pLast)
						pArgs++;
					else
						break;				
				}

				return nArgs;
				#pragma endregion
			}
			else if (pArgs->Text == ".half")
			{
				#pragma region HALF
				pArgs++;

				if (!pStack->Align(2))
					return _UI32_MAX;

				while (long long val = strtoll(pArgs->Text.data(), nullptr, 0))
				{

					if (val == 0 && pArgs->Text[0] != '0')
						break;

					if (val < _I16_MIN || val > _UI16_MAX)
					{
						std::cout << "ERROR : Value " << val << " cannot be stored in a half\n";
						return _UI32_MAX;
					}

					if (val < 0)
						*pStack->Alloc<__int16>() = static_cast<__int16>(val);
					else
						*pStack->Alloc<unsigned __int16>() = static_cast<unsigned __int16>(val);

					nArgs++;

					if (pArgs != pLast)
						pArgs++;
					else
						break;
				}

				return nArgs;
				#pragma endregion
			}
			else if (pArgs->Text == ".word")
			{
				#pragma region WORD
				pArgs++;

				if (!pStack->Align(4))
					return _UI32_MAX;

				while (long long val = strtoll(pArgs->Text.data(), nullptr, 0))
				{

					if (val == 0 && pArgs->Text[0] != '0')
						break;

					if (val < _I32_MIN || val > _UI32_MAX)
					{
						std::cout << "ERROR : Value " << val << " cannot be stored in a word\n";
						return _UI32_MAX;
					}

					if (val < 0)
						*pStack->Alloc<__int32>() = static_cast<__int32>(val);
					else
						*pStack->Alloc<unsigned __int32>() = static_cast<unsigned __int32>(val);

					nArgs++;

					if (pArgs != pLast)
						pArgs++;
					else
						break;
				}

				return nArgs;
				#pragma endregion
			}
			else if (pArgs->Text == ".space")
			{
				#pragma region SPACE
				pArgs++;

				if (!pStack->Align(4))
				{
					pStack->Resize(pStack->Capacity() * 2);
					pStack->Align(4);
				}

				while (long long val = strtoll(pArgs->Text.data(), nullptr, 0))
				{
					if (val == 0 && pArgs->Text[0] != '0')
						break;

					// Must be a postive number of bytes and less than 4gb
					if (val > 0 && val < _UI32_MAX)
					{
						pStack->Alloc(static_cast<size_t>(val));
						
						nArgs++;

						if (pArgs != pLast)
							pArgs++;
						else
							break;
					}
					else
					{
						std::cout << "ERROR : Cannot create space of size " << val << " \tbytes\n";
						return _UI32_MAX;
					}
				}

				return nArgs;
				#pragma endregion
			}
			else if (pArgs->Text == ".ascii" || pArgs->Text == ".asciiz")
			{
				#pragma region ASCII(Z)
				bool is_ascii = (pArgs->Text == ".ascii");

				pArgs++;

				if (pArgs->Text[0] != '\"' || pArgs->Text.length() < 2 || pArgs->Text[pArgs->Text.length() - 1] != '\"')
				{
					std::cout << "ERROR : String " << pArgs->Text << " is not valid\n";
					return _UI32_MAX;
				}

				nArgs++;

				std::string const val = pArgs->Text.substr(1, pArgs->Text.length() - 2);
				char * buffer = reinterpret_cast<char*>(malloc(val.length() + 1));
				memset(buffer, 0x00, val.length() + 1);

				int index = 0;

				for (unsigned __int32 i = 0; i < val.length(); i++)
				{
					if (val[i] == '\\')
					{
						if (i++ == val.length())
							break;

						switch (val[i])
						{
						case '\\':
							buffer[index] = '\\';
							break;
						case 'n':
							buffer[index] = '\n';
							break;
						case 't':
							buffer[index] = '\t';
							break;
						case '\"':
							buffer[index] = '\"';
							break;
						case '0':
							buffer[index] = 0;
							break;
						default:
							buffer[index] = '?';
							break;
						}
					}
					else
						buffer[index] = val[i];

					index++;
				}

				if (is_ascii)
					memcpy(pStack->Alloc(index), buffer, index);
				else // .asciiz
					memcpy(pStack->Alloc(index + 1), buffer, index + 1);

				return nArgs;
				#pragma endregion
			}

			return _UI32_MAX;
		}
	
		unsigned __int32 Builder::GenBss(Expression const *  pArgs, Expression const * const pLast)
		{
			int nArgs = 0;

			if (pArgs == pLast)		// Check for end of litearture
				return _UI32_MAX;

			// Is there a label?
			// If so we need to align some things
			#pragma region LABEL
			if (pArgs->Text[pArgs->Text.length() - 1] == ':')
			{
				if (pArgs + 1 == pLast)		// Check for end of litearture
					return _UI32_MAX;

				//  Align to data sizes
				if (pArgs[1].Text == ".half")
				{
					s_bss += s_bss & 0x01;
				}
				else if (pArgs[1].Text == ".word" || pArgs[1].Text == ".space")
				{
					if (s_bss & 0x3)	// Align to word
						s_bss += 4 - (s_bss & 0x3);
				}

				// Add label to list
				if (!AddLabel(Label(pArgs->Text.substr(0, pArgs->Text.length() - 1), s_bss, LABEL_TYPE_BANK_B)))
					return _UI32_MAX;

				pArgs++;
				nArgs++;
			}
			#pragma endregion

			if (pArgs->Text == ".byte")
			{
			#pragma region BYTE
				s_bss++;	// "Alloc" 1 byte
				return nArgs;
			#pragma endregion
			}
			else if (pArgs->Text == ".half")
			{
			#pragma region HALF
				s_bss += 2 + (s_bss & 0x01);
				return nArgs;
			#pragma endregion
			}
			else if (pArgs->Text == ".word")
			{
			#pragma region WORD

				if (s_bss & 0x3)	// Align to word
					s_bss += 4 - (s_bss & 0x3);

				s_bss += 4; // "Alloc" 4 bytes

				return nArgs;
			#pragma endregion
			}
			else if (pArgs->Text == ".space")
			{
			#pragma region SPACE
				long val = strtol(pArgs[1].Text.data(), nullptr, 0);
				if (val > 0)
				{
					if (s_bss & 0x3)	// Align to word
						s_bss += 4 - (s_bss & 0x3);

					s_bss += val; // "Alloc" bytes
					return nArgs + 1;
				}
				else
				{
					std::cout << "ERROR : Cannot create space of size " << val << " \tbytes\n";
					return _UI32_MAX;
				}
			#pragma endregion
			}

			return _UI32_MAX;
		}

		unsigned __int32 Builder::GenTextOp(Offset<CPU_OP> op, Expression const * pArg, unsigned __int32 pType)
		{
			// What type of parameter do we expect?
			switch (pType)
			{
				case NODE_TYPE_RS:	// A source register
				{	Semantic const * p = MatchNode(pArg->Text);	// Find the corresponding register
					if (p->Type == SEMANTIC_TYPE_REG)
						op->I.rs = p->Values[0];		// Identify source register
					else
						return _UI32_MAX;				// Failed to identify register
				}	break;
				case NODE_TYPE_RT:	// A target register
				{	Semantic const * p = MatchNode(pArg->Text);	// Find the corresponding register
					if (p->Type == SEMANTIC_TYPE_REG)
						op->I.rt = p->Values[0];		// Identify target register
					else
						return _UI32_MAX;				// Failed to identify register
				}	break;
				case NODE_TYPE_RD:	// A destination register
				{	Semantic const * p = MatchNode(pArg->Text);	// Find the corresponding register
					if (p->Type == SEMANTIC_TYPE_REG)
						op->R.rd = p->Values[0];		// Identify destination register
					else
						return _UI32_MAX;				// Failed to identify register
				}	break;
				case NODE_TYPE_IMMEDIATE:	// An immediate value
					// Interpret this argument as a numeric value
					op->I.i = static_cast<__int16>(strtol(pArg->Text.data(), nullptr, 0));

					// Was this maybe supposed to be a label?
					if (op->I.i == 0 && pArg->Text[0] != '0')
					{	// Add a reference here
						References.push_back(Reference(pArg->Text, op.offset, R_MIPS_16));
					}
					break;
				case NODE_TYPE_SHAMT:	// A numeric value corresponding to a bitshift
					// Interpret this argument as a numeric value
					op->R.shamt = strtol(pArg->Text.data(), nullptr, 0);
					break;
				case NODE_TYPE_TARGET:	// An address of some sort
				{	// Interpret this argument as a numeric value and >> 2
					if (!(op->J.target = (strtol(pArg->Text.data(), nullptr, 0) >> 2)))
					{
						// Was this maybe supposed to be a label?
						Semantic const * p = MatchNode(pArg->Text);
						if (p->Type == SEMANTIC_TYPE_ADDRESS && p->Values[0])
							op->J.target = p->Values[0] >> 2;	// Woah, this label exists
						else
							References.push_back(Reference(pArg->Text, op.offset, R_MIPS_26)); // Add a reference here
					}
				}	break;
				case NODE_TYPE_OFFSET:	// A source register with an offset
				{
					// Find the location of the parenthesis, return npos if none
					size_t paren = pArg->Text.find('(');

					// Set the offset, which is the value before the parenthesis
					//  interpret this first part as a value
					long offset = strtol(pArg->Text.substr(0, paren).data(), nullptr, 0);
					if (offset < 0)	// Is this a negative offset?
						op->I.i = static_cast<__int16>(offset);	// Yes we need the signed version
					else
						op->I.u = static_cast<unsigned __int16>(offset);

					// Was this maybe NOT supposed to be zero
					if (op->I.i == 0 && pArg->Text[0] != '0')
					{
						std::cout << "ERROR : Offset must be a number - " << pArg->Text << std::endl;
						return _UI32_MAX;
					}

					// Set the source registar
					if (paren != std::string::npos)
					{
						// Find the portion of the arg that describes the register
						std::string regArg = pArg->Text.substr(paren + 1, std::string::npos);
						regArg.pop_back();	// Should remove ')'

						Semantic const * regNode = MatchNode(regArg);
						if (regNode->Type == SEMANTIC_TYPE_REG)
							op->I.rs = regNode->Values[0];	// Identify source register
						else
							return _UI32_MAX;				// Failed to identify register
					}
				}	break;
				case NODE_TYPE_RTARGET:	// A relative target address
				{
					// Interpret this argument as a numeric value
					op->I.i = static_cast<__int16>(strtol(pArg->Text.data(), nullptr, 0));

					// Was this maybe supposed to be a label?
					if (op->I.i == 0 && pArg->Text[0] != '0')
					{
						References.push_back(Reference(pArg->Text, op.offset, R_MIPS_PC16));
					}
				}	break;
			}
		}

		unsigned __int32 Builder::GenText(Expression const * pArgs)
		{
			// Find which operation this expression corresponds to
			const Semantic * const baseSem = MatchNode(pArgs->Text);

			// Keep track of the number of arguments we use
			unsigned __int32 nArgs = 0;

			// Is this an operation?
			if (baseSem->Type == SEMANTIC_TYPE_OP){
				// Allocate space for a new opcode
				Offset<CPU_OP> op (s_text.Alloc<CPU_OP>(), &s_text);
				// The first value is the inital value for the operation
				op->WORD = baseSem->Values[0];
				// The remaining values dictate what parameters are used
				for (int i = 1; i < baseSem->Values.size(); i++){			
					nArgs++;
					if (GenTextOp(op, pArgs + i, baseSem->Values[i]) == _UI32_MAX)
						return _UI32_MAX;
				}
			}
			// Is this some sort of special operation?
			else if (baseSem->Type == SEMANTIC_TYPE_OPS)
			{
				return GenTextOpS(pArgs + 1, baseSem->Values[0]);
			}
			else
				return _UI32_MAX;

			return nArgs;
		}

		unsigned __int32 Builder::GenTextOpS(Expression const * const pArgs, unsigned __int32 op)
		{
			// Decide what to do based on this special operations identifying value
			switch (op)
			{
			case(SOP_CODE_LA) :	// la
			#pragma region LA
			{
				const Semantic * const p1 = MatchNode(pArgs[0].Text);
				if (p1->Type != SEMANTIC_TYPE_REG)
					return _UI32_MAX;

				long val = strtol(pArgs[1].Text.data(), nullptr, 0);

				CPU_OP * ops = s_text.Alloc<CPU_OP>(2);
				ops[0].WORD = CPU_LUI;
				ops[1].WORD = CPU_ORI;

				ops[0].I.rt = 1;
				ops[1].I.rs = 1;
				ops[1].I.rt = p1->Values[0];

				ops[0].I.u = val >> 16;
				ops[1].I.u = val & 0xFFFF;

				// Was this maybe supposed to be a label?
				if (val == 0 && pArgs[1].Text[0] != '0')
				{
					References.push_back(Reference(pArgs[1].Text, reinterpret_cast<char*>(ops)-s_text.Begin(), R_MIPS_HI16));
					References.push_back(Reference(pArgs[1].Text, reinterpret_cast<char*>(ops + 1) - s_text.Begin(), R_MIPS_LO16));
				}

				return 2;
			}	break;
			#pragma endregion
			case(SOP_CODE_LI) :	// li
			#pragma region LI
			{
				const Semantic * const p1 = MatchNode(pArgs[0].Text);
				if (p1->Type != SEMANTIC_TYPE_REG)
					return _UI32_MAX;

				long val = strtol(pArgs[1].Text.data(), nullptr, 0);

				if (val >= _I16_MIN || val <= _UI16_MAX)
				{
					CPU_OP * ops = s_text.Alloc<CPU_OP>();
					ops[0].WORD = CPU_ADDI;
					ops[0].I.rt = p1->Values[0];
					if (val < 0)
						ops[0].I.i = static_cast<__int16>(val);
					else
						ops[0].I.u = static_cast<unsigned __int16>(val);
				}
				else
				{
					CPU_OP * ops = s_text.Alloc<CPU_OP>(2);
					ops[0].WORD = CPU_LUI;
					ops[1].WORD = CPU_ORI;

					ops[0].I.rt = 1;
					ops[1].I.rs = 1;
					ops[1].I.rt = p1->Values[0];

					ops[0].I.u = val >> 16;
					ops[1].I.u = val & 0xFFFF;
				}

				return 2;

			}	break;
			#pragma endregion
			case (SOP_CODE_PUSH) :	// push
			#pragma region PUSH
			{
				// Get the first parameter
				const Semantic * const p1 = MatchNode(pArgs[0].Text);
				if (p1->Type != SEMANTIC_TYPE_REG)	// The first paremeter better be a register
					return _UI32_MAX;

				// Allocate enough space for 3 opcodes
				CPU_OP * ops = s_text.Alloc<CPU_OP>(3);
				ops[0].WORD = CPU_LUI;
				ops[1].WORD = 0;
				ops[2].WORD = CPU_SW;

				ops[0].I.rt = 0x01;		// li $r1, 4
				ops[0].I.u = 4;

				ops[1].R.op = CPU_FUNC;	// subu $sp,$sp,$r1
				ops[1].R.funct = CPU_FUNC_SUBU;
				ops[1].R.rd = 0x1D;
				ops[1].R.rs = 0x1D;
				ops[1].R.rt = 0x01;

				ops[2].I.rt = p1->Values[0];	// sw $VAL,($sp)
				ops[2].I.rs = 0x1D;

				return 1;

			}	break;
			#pragma endregion
			case (SOP_CODE_POP) :	// pop
			#pragma region POP
			{
				Semantic const * const p1 = MatchNode(pArgs[0].Text);
				if (p1->Type != SEMANTIC_TYPE_REG)
					return _UI32_MAX;

				CPU_OP * ops = s_text.Alloc<CPU_OP>(3);
				ops[0].WORD = CPU_LUI;
				ops[1].WORD = CPU_LW;
				ops[2].WORD = 0;

				ops[0].I.rt = 0x01;		// li $r1, 4
				ops[0].I.u = 4;

				ops[1].I.rt = p1->Values[0];	// lw $VAL,($sp)
				ops[1].I.rs = 0x1D;

				ops[2].R.op = CPU_FUNC;	// subu $sp,$sp,$r1
				ops[2].R.funct = CPU_FUNC_ADDU;
				ops[2].R.rd = 0x1D;
				ops[2].R.rs = 0x1D;
				ops[2].R.rt = 0x01;

				return 1;

			}	break;
			#pragma endregion
			default :
				return _UI32_MAX;
			}
		}

		bool Builder::GenELF(char const * const pFile)
		{
			std::cout << "\nGenerating ELF object\n";

			DynamicStackAlloc ELF(512);
			DynamicStackAlloc s_hdr(128);	// Temp storage for section headers
			
			Elf32_Half cursec = 1;	// Tracks current Elf32_Shdr index, starts at 1 because UNDEF section = 0
		#pragma region EHDR
			Offset<Elf32_Ehdr> o_ehdr (ELF.Offset(), &ELF);
			memset(ELF.Alloc<Elf32_Ehdr>(), 0x00, sizeof(Elf32_Ehdr));	// Zero header
			o_ehdr->e_ident[EI_MAG0] = ELFMAG0;	// Magic 0x7f
			o_ehdr->e_ident[EI_MAG1] = ELFMAG1;	// 'E'
			o_ehdr->e_ident[EI_MAG2] = ELFMAG2;	// 'L'
			o_ehdr->e_ident[EI_MAG3] = ELFMAG3;	// 'F'
			o_ehdr->e_ident[EI_CLASS]	= 1;	// 32 bit
			o_ehdr->e_ident[EI_DATA]		= 1;	// litle endian
			o_ehdr->e_ident[EI_VERSION]	= 1;	// ELF version 1
			o_ehdr->e_type = ET_REL;
			o_ehdr->e_machine = EM_MIPS;
			o_ehdr->e_version = EV_CURRENT;
			o_ehdr->e_flags = EF_MIPS_PIC;
			o_ehdr->e_ehsize = sizeof(Elf32_Ehdr);
			o_ehdr->e_shentsize = sizeof(Elf32_Shdr);
		#pragma endregion

		#pragma region UNDEFINED_SECTION
			memset(s_hdr.Alloc<Elf32_Shdr>(), 0x00, sizeof(Elf32_Shdr));	// Add UNDEF section
		#pragma endregion

		#pragma region TEXT
			Offset<CPU_OP>		o_text;
			Offset<Elf32_Shdr>	o_text_hdr;
			Elf32_Half si_text = 0;
			if (s_text.Offset())	// Store .text data in elf and create eshdr
			{
				// data
				o_text = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_text.Offset()), s_text.Begin(), s_text.Offset());
						
				// shdr
				o_text_hdr = { s_hdr.Offset(), &s_hdr };
				memset(s_hdr.Alloc<Elf32_Shdr>(), 0x00, sizeof(Elf32_Shdr));
				si_text = cursec++;

				o_text_hdr->sh_name			= AddString(".text");
				o_text_hdr->sh_type			= SHT_PROGBITS;
				o_text_hdr->sh_flags		= SHF_ALLOC | SHF_EXECINSTR;
				o_text_hdr->sh_offset		= o_text.offset;
				o_text_hdr->sh_size			= s_text.Offset();
				o_text_hdr->sh_addralign	= 0x04;

				// Print progress
				std::cout << ">> .text\t: " << o_text_hdr->sh_size << " \tbytes\n";
			}
		#pragma endregion

		#pragma region DATA
			if (!ELF.Align(4))	// Align to word
				return false;
			
			Offset<char> o_data;
			Offset<Elf32_Shdr>	o_data_hdr;
			Elf32_Half si_data = 0;
			if (s_data.Offset())	// Store .data data in elf and create shdr
			{
				// data
				o_data = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_data.Offset()), s_data.Begin(), s_data.Offset());

				// shdr
				o_data_hdr = { s_hdr.Offset(), &s_hdr };
				memset(s_hdr.Alloc<Elf32_Shdr>(), 0x00, sizeof(Elf32_Shdr));
				si_data = cursec++;

				o_data_hdr->sh_name			= AddString(".data");
				o_data_hdr->sh_type			= SHT_PROGBITS;
				o_data_hdr->sh_flags		= SHF_WRITE | SHF_ALLOC;
				o_data_hdr->sh_offset		= o_data.offset;
				o_data_hdr->sh_size			= s_data.Offset();
				o_data_hdr->sh_addralign	= 0x04;	

				// Print progress
				std::cout << ">> .data\t: " << o_data_hdr->sh_size << " \tbytes\n";
			}
		#pragma endregion

		#pragma region RODATA
			if (!ELF.Align(4))	// Align to word
				return false;

			Offset<char> o_rodata;
			Offset<Elf32_Shdr>	o_rodata_hdr;
			Elf32_Half si_rodata = 0;
			if (s_rodata.Offset())	// Store .rodata data in elf and create shdr
			{
				// data
				o_rodata = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_rodata.Offset()), s_rodata.Begin(), s_rodata.Offset());

				// shdr
				o_rodata_hdr = { s_hdr.Offset(), &s_hdr };
				memset(s_hdr.Alloc<Elf32_Shdr>(), 0x00, sizeof(Elf32_Shdr));
				si_rodata = cursec++;

				o_rodata_hdr->sh_name		= AddString(".rodata");
				o_rodata_hdr->sh_type		= SHT_PROGBITS;
				o_rodata_hdr->sh_offset		= o_rodata.offset;
				o_rodata_hdr->sh_flags		= SHF_ALLOC;
				o_rodata_hdr->sh_size		= s_rodata.Offset();
				o_rodata_hdr->sh_addralign	= 0x04;

				// Print progress
				std::cout << ">> .rodata\t: " << o_rodata_hdr->sh_size << " \tbytes\n";
			}
		#pragma endregion

		#pragma region BSS
			if (!ELF.Align(4))	// Align to word
				return false;

			Offset<Elf32_Shdr>	o_bss_hdr;
			Elf32_Half si_bss = 0;
			if (s_bss)	// Create .bss shdr
			{
				// shdr
				o_bss_hdr = { s_hdr.Offset(), &s_hdr };
				memset(s_hdr.Alloc<Elf32_Shdr>(), 0x00, sizeof(Elf32_Shdr));
				si_bss = cursec++;

				o_bss_hdr->sh_name			= AddString(".bss");
				o_bss_hdr->sh_type			= SHT_NOBITS;
				o_bss_hdr->sh_flags			= SHF_WRITE | SHF_ALLOC;
				o_bss_hdr->sh_size			= s_bss;
				o_bss_hdr->sh_addralign		= 0x04;

				// Print progress
				std::cout << ">> .bss\t\t: " << o_bss_hdr->sh_size << " \tbytes*\n";
			}
		#pragma endregion

		#pragma region SYMTAB
			if (!ELF.Align(4))	// Align to word
				return false;

			Offset<Elf32_Sym> o_symtab;
			Offset<Elf32_Shdr>	o_symtab_hdr;
			Elf32_Half si_symtab = 0;
			if (Labels.size())	//
			{
				Elf32_Off o_globals = GenSymtab(si_text, si_data, si_rodata, si_bss);

				// data
				o_symtab = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_symtab.Offset()), s_symtab.Begin(), s_symtab.Offset());

				// shdr
				o_symtab_hdr = { s_hdr.Offset(), &s_hdr };
				memset(s_hdr.Alloc<Elf32_Shdr>(), 0x00, sizeof(Elf32_Shdr));
				si_symtab = cursec++;

				o_symtab_hdr->sh_name		= AddString(".symtab");
				o_symtab_hdr->sh_type		= SHT_SYMTAB;
				o_symtab_hdr->sh_offset		= o_symtab.offset;
				o_symtab_hdr->sh_size		= s_symtab.Offset();
				o_symtab_hdr->sh_addralign	= 0x04;
				o_symtab_hdr->sh_entsize	= sizeof(Elf32_Sym);

				// Print progress
				std::cout << ">> .symtab\t: " << o_symtab_hdr->sh_size << " \tbytes\n";
			}
		#pragma endregion

		#pragma region RELA_TEXT
			if (!ELF.Align(4))	// Align to word
				return false;

			Offset<Elf32_Rela>	o_rela;
			Offset<Elf32_Shdr>	o_rela_hdr;
			Elf32_Half si_rela = 0;
			if (References.size())	// Store rela data in elf and create eshdr
			{
				GenRela();	// Generates .rela data

				// data
				o_rela = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_rela.Offset()), s_rela.Begin(), s_rela.Offset());

				// shdr
				o_rela_hdr = { s_hdr.Offset(), &s_hdr };
				memset(s_hdr.Alloc<Elf32_Shdr>(), 0x00, sizeof(Elf32_Shdr));
				si_rela = cursec++;

				o_rela_hdr->sh_name = AddString(".rela.text");
				o_rela_hdr->sh_type = SHT_RELA;
				o_rela_hdr->sh_flags = 0;
				o_rela_hdr->sh_offset = o_rela.offset;
				o_rela_hdr->sh_size = s_rela.Offset();
				o_rela_hdr->sh_addralign = 0x04;
				o_rela_hdr->sh_link = si_symtab;
				o_rela_hdr->sh_info = si_text;

				// Print progress
				std::cout << ">> .rela.text\t: " << o_rela_hdr->sh_size << " \tbytes\n";
			}
		#pragma endregion

		#pragma region STRTAB
			Offset<char>		o_strtab;
			Offset<Elf32_Shdr>	o_strtab_hdr;
			Elf32_Half si_strtab = 0;
			if (s_strtab.Offset())	// Store .strtab data in elf and create eshdr
			{
				// shdr
				o_strtab_hdr = { s_hdr.Offset(), &s_hdr };
				memset(s_hdr.Alloc<Elf32_Shdr>(), 0x00, sizeof(Elf32_Shdr));
				
				si_strtab = cursec++;

				o_strtab_hdr->sh_name	= AddString(".strtab");
				o_strtab_hdr->sh_type = SHT_STRTAB;
				o_strtab_hdr->sh_flags = 0;

				// data
				o_strtab = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_strtab.Offset()), s_strtab.Begin(), s_strtab.Offset());

				// shdr				
				o_strtab_hdr->sh_offset		= o_strtab.offset;
				o_strtab_hdr->sh_size		= s_strtab.Offset();
				o_strtab_hdr->sh_addralign	= 0x1;

				// Update elf header
				o_ehdr->e_shstrndx = si_strtab;

				// Print progress
				std::cout << ">> .strtab\t: " << o_strtab_hdr->sh_size << " \tbytes\n";
			}
		#pragma endregion

		#pragma region SHDR
			Offset<Elf32_Shdr>	o_shdr;
			{
				o_shdr = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_hdr.Offset()), s_hdr.Begin(), s_hdr.Offset());

				// Update elf header
				o_ehdr->e_shoff = o_shdr.offset;
				o_ehdr->e_shnum = cursec;

				std::cout << ">> sec headers\t: " << s_hdr.Offset() << " \tbytes\n";
			}
		#pragma endregion

			// Print progress
			std::cout << ">> TOTAL SIZE\t: " << ELF.Offset() << " \tbytes\n";

			std::ofstream fout(pFile, std::ios::out | std::ios::binary | std::ios::trunc);
			if (!fout.is_open())
			{
				std::cout << "ERROR : Unable to write to file " << pFile << std::endl;
				return false;
			}

			fout.write(ELF.Begin(), ELF.Offset());
			fout.close();

			// Print progress
			std::cout << ">> Saved ELF object as \"" << pFile << "\"" << std::endl;

			return true;
		}

		Elf32_Off Builder::GenSymtab(Elf32_Half const si_text, Elf32_Half const si_data, Elf32_Half const si_rodata, Elf32_Half const si_bss)
		{
			// Mark Global Symbols
			for (unsigned __int32 l = 0; l < Labels.size(); l++)
			{
				for (unsigned __int32 g = 0; g < Globals.size(); g++)	// Is this a global label
				{
					// Is there a Global with the same ID?
					if (Labels[l].ID == Globals[g].ID)
					{
						Globals[g].iLabel = l;		// Reference label
						Labels[l].SetGlobal(true);	// Denote that this is a "global" label	
						break;
					}
				}
			}

			// Detect Undefined Labels
			for (unsigned __int32 r = 0; r < References.size(); r++)
			{
				bool defined = false;	// Find a matching label
				for (unsigned __int32 l = 0; l < Labels.size(); l++)
				{
					if (Labels[l].ID == References[r].ID)	// Does this one match?
					{
						defined = true;	// Aha! We are defined
						break;
					}
				}

				if (!defined)	// Still not defined, add a new reference
					AddLabel(Label(References[r].ID));
			}

			Elf32_Off o_globals = 0;

			// Add Local and Undefined Labels
			for (unsigned __int32 l = 0; l < Labels.size(); l++)
			{
				if (!Labels[l].IsGlobal())
				{
					o_globals++;

					Elf32_Sym* pSym = s_symtab.Alloc<Elf32_Sym>();
					pSym->st_name	= AddString(Labels[l].ID.c_str());
					pSym->st_value	= Labels[l].Offset;
					pSym->st_size	= 0;
					pSym->st_other	= 0;

					switch (Labels[l].GetBank())
					{
					case(LABEL_TYPE_BANK_T) :
						pSym->st_shndx = si_text;
						pSym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_FUNC);
						break;
					case(LABEL_TYPE_BANK_D) :
						pSym->st_shndx = si_data;
						pSym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
						break;
					case(LABEL_TYPE_BANK_R) :
						pSym->st_shndx = si_rodata;
						pSym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
						break;
					case(LABEL_TYPE_BANK_B) :
						pSym->st_shndx = si_bss;
						pSym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
						break;
					default:
						pSym->st_shndx = SHN_UNDEF;
						pSym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_NOTYPE);
						break;
					}
				}
			}
	
			if (Globals.size())
			{
				// Add Global Labels
				for (unsigned __int32 l = 0; l < Labels.size(); l++)
				{
					if (Labels[l].IsGlobal())
					{
						Elf32_Sym* pSym = s_symtab.Alloc<Elf32_Sym>();
						pSym->st_name = AddString(Labels[l].ID.c_str());
						pSym->st_value = Labels[l].Offset;
						pSym->st_size = 0;
						pSym->st_other = 0;

						switch (Labels[l].GetBank())
						{
						case(LABEL_TYPE_BANK_T) :
							pSym->st_shndx = si_text;
							pSym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
							break;
						case(LABEL_TYPE_BANK_D) :
							pSym->st_shndx = si_data;
							pSym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT);
							break;
						case(LABEL_TYPE_BANK_R) :
							pSym->st_shndx = si_rodata;
							pSym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT);
							break;
						case(LABEL_TYPE_BANK_B) :
							pSym->st_shndx = si_bss;
							pSym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT);
							break;
						default:
							pSym->st_shndx = SHN_UNDEF;
							pSym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE);
							break;
						}
					}
				}
			}


			return o_globals;
		}

		void Builder::GenRela(void)
		{
			Elf32_Sym const * const pSym = reinterpret_cast<Elf32_Sym*>(s_symtab.Begin());
			Elf32_Sym const * const pEnd = reinterpret_cast<Elf32_Sym*>(s_symtab.End());

			for (unsigned __int32 r = 0; r < References.size(); r++)
			{
				for (unsigned __int32 s = 0; pSym + s != pEnd; s++)
				{
					if (strcmp(References[r].ID.c_str(), s_strtab.Begin() + pSym[s].st_name) == 0)
					{
						Elf32_Rela * p_rela = s_rela.Alloc<Elf32_Rela>();
						p_rela->r_offset = References[r].Offset;
						p_rela->r_info = ELF32_R_INFO(s, References[r].Type);
						p_rela->r_addend = 0;
						break;
					}
				}
			}
		}

		Elf32_Word Builder::AddString(char const * const pString)
		{
			// Return offset to existing string
			for (char * pOld = s_strtab.Begin(); pOld != s_strtab.Cursor();)
			{
				if (strcmp(pOld, pString) == 0)
					return pOld - s_strtab.Begin();

				pOld += strlen(pOld) + 1;
			}

			// Add new string
			size_t strSize = strlen(pString) + 1;
			return (reinterpret_cast<char*>(memcpy(s_strtab.Alloc(strSize), pString, strSize)) - s_strtab.Begin());
		}
	}
}