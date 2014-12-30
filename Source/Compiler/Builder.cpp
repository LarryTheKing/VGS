#include "Builder.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "..\CPU_OP.h"
#include "Offset.h"

namespace VGS
{
	namespace Compiler
	{

		Builder::Builder(char const * const fLang)
			: s_text(128), s_data(128), s_rodata(128), s_bss(0), s_strtab(128), s_symtab(128), s_rela(128)
		{
			LoadLanguage(fLang);
		}

		Builder::~Builder()
		{
			Reset();
		}

		void Builder::Reset(void)
		{
			Litterature.clear();
			s_text.Reset();
			s_data.Reset();
			s_rodata.Reset();
			s_bss = 0;
			s_strtab.Reset(); *s_strtab.Alloc<char>() = 0;	// First byte is always '\0'
			s_symtab.Reset();
			s_rela.Reset();
		}

		bool Builder::Build(char const * const fin, char const * const fout)
		{
			std::cout << "\nBuilding : \"" << fin << "\"\n";

			Reset();
			if (!GenLitTree(fin))
			{
				std::cout << "ERROR : Failed to create litterature tree\n";
				return false;
			}

			if (!GenProgTree())
			{
				std::cout << "ERROR : Failed to create program tree\n";
				return false;
			}

			if (!GenELF(fout))
			{
				std::cout << "ERROR : Failed to create elf object\n";
				return false;
			}

			return true;
		}

		void Builder::LoadLanguage(char const * const fileName)
		{
			std::ifstream ifile(fileName);
			std::cout << "Opening Language File : \"" << fileName << "\"\n";
			if (!ifile.is_open())
			{
				std::cout << "ERROR : Failed to open Language File" << std::endl;
				return;
			}

			ProcNode node = { "UNDEFINED", 0x0, 0x0 };
			Language.push_back(node);

			while ((ifile >> node.ID >> std::hex >> node.i.Value >> std::hex >> node.i.Type))
			{
				Language.push_back(node);
			}

			std::cout << ">> Found " << Language.size() << " terms\n";
		}
		
		bool Builder::GenLitTree(char const * const fileName)
		{
			std::ifstream ifile(fileName);
			std::cout << "Opening file \"" << fileName << "\"\n";
			if (!ifile.is_open())
			{
				std::cout << "ERROR : Failed to open file." << std::endl;
				return false;
			}

			// Read each line from file and generate litterature tree

			std::cout << "\nGenerating litterature tree...\n";

			unsigned __int32 nLine = 1;

			std::string	sLine;
			while (std::getline(ifile, sLine))
			{
				if (!GenLitNode(sLine))
				{
					std::cout << "ERROR : Unable to parse line " << nLine << std::endl;
					return false;
				}
				nLine++;
			}

			std::cout << ">> Found " << Litterature.size() << " words\n";

			return true;
			// TODO : Allow include files
		}
		
		bool Builder::GenLitNode(std::string const sLine)
		{
			if (!sLine[0])		// Skip empty lines
				return true;

			int		index = 0;
			bool	bQuote = false;

			for (size_t i = 0; i < sLine.size() + 1; i++)	// Parse line
			{
				if (sLine[i] == '\"')
				{
					if (bQuote && sLine[i - 1] != '\\')
					{

						bQuote = false;
					}
					else
						bQuote = true;
				}
				else if ((!bQuote && (sLine[i] == ' ' || sLine[i] == ',' || sLine[i] == '\t')) || sLine[i] == '\0')
				{
					if (i > index)
					{
						Litterature.push_back(sLine.substr(index, i - index));
					}

					index = i + 1;

					if (!sLine[i])
						return true;
				}
				else if (sLine[i] == '#' || sLine[i] == '\n')
					return true;
			}

			return false;
		}

		bool ValidateName(const std::string & s)
		{
			if (s[0] > 64 && s[0] < 91)
				return true;
			else if (s[0] > 96 && s[0] < 123)
				return true;
			else if (s[0] == 95)
				return true;

			return false;
		}

		bool Builder::AddGlobal(std::string const s)
		{
			if (!ValidateName(s))
				return false;

			for (unsigned __int32 i = 0; i < Globals.size(); i++)
			{	
				if (s == Globals[i].ID)
					return true;
			}

			Globals.push_back(ProcNode(s, 0, 0));
			return true;
		}

		bool Builder::AddLabel(ProcNode const n)
		{
			if (!ValidateName(n.ID))
				return false;

			for (int i = 0; i < Labels.size(); i++)
			{
				if (n.ID == Labels[i].ID)
				{
					if (n.i.Value != _UI32_MAX)
						std::cout << "ERROR : Label redefinition -" << n.ID << std::endl;

					return false;
				}
			}

			Labels.push_back(n);
			return true;
		}

		iNode Builder::MatchNode(const std::string & rString)
		{
			const ProcNode * pN = Language.data();
			const size_t length = Language.size();
			for (size_t i = 1; i < length; i++)
			{
				// Check for match
				if (rString == Language[i].ID)
					return Language[i].i;
			}

			// No match found, return UNDEFINEd node
			return Language[0].i;
		}

#define MODE_TEXT	0x01
#define MODE_DATA	0x02
#define MODE_RODATA	0x03
#define MODE_BSS	0x04

		bool Builder::GenProgTree(void)
		{
			std::cout << "\nGenerating program tree...\n";

			unsigned __int32 mode = MODE_TEXT;

			const size_t length = Litterature.size();

			for (size_t i = 0; i < length; i++)
			{
				std::string const cWord = Litterature[i];
				if (cWord == ".text")
					mode = MODE_TEXT;
				else if (cWord == ".data")
					mode = MODE_DATA;
				else if (cWord == ".rodata")
					mode = MODE_RODATA;
				else if (cWord == ".bss")
					mode = MODE_RODATA;
				else if (cWord == ".global")
				{
					if (!AddGlobal(Litterature[++i]))
						return false;
				}
				else if (mode == MODE_TEXT)
				{
					if (cWord[cWord.length() - 1] == ':')
					{
						if (!AddLabel(ProcNode(cWord.substr(0, cWord.length() - 1), s_text.Offset(), NODE_TYPE_OFFSET_T)))
							return false;
					}
					else
					{
						unsigned __int32 const s = GenText(&Litterature[i]);
						if (_UI32_MAX != s)
							i += s;
						else
							return false;
					}
				}
				else if (mode == MODE_DATA || mode == MODE_RODATA)
				{
					unsigned __int32 const s = GenData(&Litterature[i], &Litterature.back(), mode);
					if (_UI32_MAX != s)
						i += s;
					else
						return false;
				}
				else if (mode == MODE_BSS)
				{
					unsigned __int32 const s = GenBss(&Litterature[i], &Litterature.back());
					if (_UI32_MAX != s)
						i += s;
					else
						return false;
				}
			}

			return true;
		}

		unsigned __int32 Builder::GenData(std::string const * pArgs, std::string const * const pLast, unsigned __int32 mode)
		{
			DynamicStackAlloc * pStack = nullptr;

			if (mode == MODE_DATA)
				pStack = &s_data;
			else if (mode == MODE_RODATA)
				pStack = &s_rodata;
			else
				return _UI32_MAX;

			int nArgs = 0;

			if (pArgs == pLast)		// Check for end of litearture
				return _UI32_MAX;

			// Is there a label?
			#pragma region LABEL
			if (pArgs[0][pArgs[0].length() - 1] == ':')
			{
				if (pArgs + 1 == pLast)		// Check for end of litearture
					return _UI32_MAX;

				//  Align to data sizes
				if (pArgs[1] == ".half")
				{
					if (!pStack->Align(2))
						return _UI32_MAX;
				}
				else if (pArgs[1] == ".word" || pArgs[1] == ".space")
				{
					if (!pStack->Align(4))
						return _UI32_MAX;
				}

				// Add label to list
				if (!AddLabel(ProcNode(pArgs[0].substr(0, pArgs[0].length() - 1), pStack->Offset(), (mode == MODE_DATA ? NODE_TYPE_OFFSET_D : NODE_TYPE_OFFSET_R))))
					return _UI32_MAX;
				
				pArgs++;
				nArgs++;
			}
			#pragma endregion

			if (*pArgs == ".byte")
			{
				#pragma region BYTE
				pArgs++;

				while (long val = strtol(pArgs->data(), nullptr, 0) && (val != 0 || pArgs[0][0] == '0'))
				{
					if (val < _I8_MIN || val > _UI8_MAX)
					{
						std::cout << "ERROR : Value " << val << " cannot be stored in a byte\n";
						return _UI32_MAX;
					}

					if (val < 0)
						*pStack->Alloc<__int8>() = val;
					else
						*pStack->Alloc<unsigned __int8>() = val;

					nArgs++;

					if (pArgs != pLast)
						pArgs++;
					else
						break;				
				}

				return nArgs;
				#pragma endregion
			}
			else if (*pArgs == ".half")
			{
				#pragma region HALF
				pArgs++;

				if (!pStack->Align(2))
					return _UI32_MAX;

				while (long val = strtol(pArgs->data(), nullptr, 0) && (val != 0 || pArgs[0][0] == '0'))
				{
					if (val < _I16_MIN || val > _UI16_MAX)
					{
						std::cout << "ERROR : Value " << val << " cannot be stored in a half\n";
						return _UI32_MAX;
					}

					if (val < 0)
						*pStack->Alloc<__int16>() = val;
					else
						*pStack->Alloc<unsigned __int16>() = val;

					nArgs++;

					if (pArgs != pLast)
						pArgs++;
					else
						break;
				}

				return nArgs;
				#pragma endregion
			}
			else if (pArgs[0] == ".word")
			{
				#pragma region WORD
				pArgs++;

				if (!pStack->Align(4))
					return _UI32_MAX;

				while (long val = strtol(pArgs->data(), nullptr, 0) && (val != 0 || pArgs[0][0] == '0'))
				{
					if (val < _I32_MIN || val > _UI32_MAX)
					{
						std::cout << "ERROR : Value " << val << " cannot be stored in a word\n";
						return _UI32_MAX;
					}

					if (val < 0)
						*pStack->Alloc<__int32>() = val;
					else
						*pStack->Alloc<unsigned __int32>() = val;

					nArgs++;

					if (pArgs != pLast)
						pArgs++;
					else
						break;
				}

				return nArgs;
				#pragma endregion
			}
			else if (pArgs[0] == ".space")
			{
				#pragma region SPACE
				pArgs++;

				if (!pStack->Align(4))
				{
					pStack->Resize(pStack->Capacity() * 2);
					pStack->Align(4);
				}

				while (long val = strtol(pArgs->data(), nullptr, 0) && (val != 0 || pArgs[0][0] == '0'))
				{
					if (val > 0)
					{
						pStack->Alloc(val);
						
						nArgs++;

						if (pArgs != pLast)
							pArgs++;
						else
							break;
					}
					else
					{
						std::cout << "ERROR : Cannot create space of size " << val << " bytes\n";
						return _UI32_MAX;
					}
				}

				return nArgs;
				#pragma endregion
			}
			else if (pArgs[0] == ".ascii" || pArgs[0] == ".asciiz")
			{
				#pragma region ASCII(Z)
				bool is_ascii = (*pArgs == ".ascii");

				pArgs++;

				if ((*pArgs)[0] != '\"' || pArgs->length() < 2 || (*pArgs)[pArgs->length() - 1] != '\"')
				{
					std::cout << "ERROR : String " << *pArgs << " is not valid\n";
					return _UI32_MAX;
				}

				nArgs++;

				std::string const val = pArgs->substr(1, pArgs->length() - 2);
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
	
		unsigned __int32 Builder::GenBss(std::string const *  pArgs, std::string const * const pLast)
		{
			int nArgs = 0;

			if (pArgs == pLast)		// Check for end of litearture
				return _UI32_MAX;

			// Is there a label?
			#pragma region LABEL
			if (pArgs[0][pArgs[0].length() - 1] == ':')
			{
				if (pArgs + 1 == pLast)		// Check for end of litearture
					return _UI32_MAX;

				//  Align to data sizes
				if (pArgs[1] == ".half")
				{
					s_bss += s_bss & 0x01;
				}
				else if (pArgs[1] == ".word" || pArgs[1] == ".space")
				{
					if (s_bss & 0x3)	// Align to word
						s_bss += 4 - (s_bss & 0x3);
				}

				// Add label to list
				if (!AddLabel(ProcNode(pArgs[0].substr(0, pArgs[0].length() - 1), s_bss, NODE_TYPE_OFFSET_B)))
					return _UI32_MAX;

				pArgs++;
				nArgs++;
			}
			#pragma endregion

			if (pArgs[0] == ".byte")
			{
			#pragma region BYTE
				s_bss++;	// "Alloc" 1 byte
				return nArgs;
			#pragma endregion
			}
			else if (pArgs[0] == ".half")
			{
			#pragma region HALF
				s_bss += 2 + (s_bss & 0x01);
				return nArgs;
			#pragma endregion
			}
			else if (pArgs[0] == ".word")
			{
			#pragma region WORD

				if (s_bss & 0x3)	// Align to word
					s_bss += 4 - (s_bss & 0x3);

				s_bss += 4; // "Alloc" 4 bytes

				return nArgs;
			#pragma endregion
			}
			else if (pArgs[0] == ".space")
			{
			#pragma region SPACE
				long val = strtol(pArgs[1].data(), nullptr, 0);
				if (val > 0)
				{
					if (s_bss & 0x3)	// Align to word
						s_bss += 4 - (s_bss & 0x3);

					s_bss += val; // "Alloc" bytes
					return nArgs + 1;
				}
				else
				{
					std::cout << "ERROR : Cannot create space of size " << val << " bytes\n";
					return _UI32_MAX;
				}
			#pragma endregion
			}

			return _UI32_MAX;
		}

		unsigned __int32 Builder::GenText(std::string const * pArgs)
		{
			iNode baseNode = MatchNode(pArgs[0]);

			if (baseNode.t8 == NODE_TYPE_OP_S)
				return GenTextOpS(pArgs + 1, baseNode.Value);
			if (baseNode.t8 != NODE_TYPE_OP)
				return _UI32_MAX;


			CPU_OP * op = s_text.Alloc<CPU_OP>();
			op->WORD = baseNode.Value;

			baseNode.Type = baseNode.Type >> 8;

			int nArgs = 0;

			while (unsigned __int8 t = baseNode.t8)
			{
				nArgs++;
				baseNode.Type = baseNode.Type >> 8;

				switch (t)
				{
				case NODE_TYPE_RS:
				{	iNode p = MatchNode(pArgs[nArgs]);
				if (p.t8 == NODE_TYPE_REGISTER)
					op->I.rs = p.Value;
				}	break;
				case NODE_TYPE_RT:
				{	iNode p = MatchNode(pArgs[nArgs]);
				if (p.t8 == NODE_TYPE_REGISTER)
					op->I.rt = p.Value;
				}	break;
				case NODE_TYPE_RD:
				{	iNode p = MatchNode(pArgs[nArgs]);
				if (p.t8 == NODE_TYPE_REGISTER)
					op->R.rd = p.Value;
				}	break;
				case NODE_TYPE_IMMEDIATE:
					op->I.i = strtol(pArgs[nArgs].data(), nullptr, 0);

					// Was this maybe supposed to be a label?
					if (op->I.i == 0 && pArgs[nArgs][0] != '0')
					{
						References.push_back(ProcNode(pArgs[nArgs], reinterpret_cast<char*>(op)-s_text.Begin(), R_MIPS_16));
					}
					break;
				case NODE_TYPE_SHAMT:
					op->R.shamt = strtol(pArgs[nArgs].data(), nullptr, 0);
					break;
				case NODE_TYPE_TARGET:
				{	if (!(op->J.target = (strtol(pArgs[nArgs].data(), nullptr, 0) >> 2)))
				{
					iNode p = MatchNode(pArgs[nArgs]);
					if (p.Type == NODE_TYPE_ADDRESS && p.Value)
						op->J.target = p.Value >> 2;
					else
						References.push_back(ProcNode(pArgs[nArgs], reinterpret_cast<char*>(op)-s_text.Begin(), R_MIPS_26));
				}
				}	break;
				case NODE_TYPE_OFFSET:
				{

					size_t paren = pArgs[nArgs].find('(');

					// Set offset
					long offset = strtol(pArgs[nArgs].substr(0, paren).data(), nullptr, 0);
					if (offset < 0)
						op->I.i = offset;
					else
						op->I.u = offset;

					// Was this maybe supposed to be zero
					if (op->I.i == 0 && pArgs[nArgs][0] != '0')
					{
						std::cout << "ERROR : Offset must be a number - " << pArgs[nArgs] << std::endl;
						return _UI32_MAX;
					}

					if (paren != std::string::npos)
					{
						std::string regArg = pArgs[nArgs].substr(paren + 1, std::string::npos);
						regArg.pop_back();	// Should remove ')'
						iNode regNode = MatchNode(regArg);
						if (regNode.t8 != NODE_TYPE_REGISTER)
							return _UI32_MAX;

						op->I.rs = regNode.Value;
					}

				}	break;
				case NODE_TYPE_RTARGET:
				{
					op->I.i = strtol(pArgs[nArgs].data(), nullptr, 0);

					// Was this maybe supposed to be a label?
					if (op->I.i == 0 && pArgs[nArgs][0] != '0')
					{
						References.push_back(ProcNode(pArgs[nArgs], reinterpret_cast<char*>(op)-s_text.Begin(), R_MIPS_PC16));
					}
				}	break;

				}
			}

			return nArgs;
		}

		unsigned __int32 Builder::GenTextOpS(std::string const * const pArgs, unsigned __int32 op)
		{
			switch (op)
			{
			case(0x01) :	// la
			#pragma region LA
			{
				iNode p1 = MatchNode(pArgs[0]);
				if (p1.t8 != NODE_TYPE_REGISTER)
					return _UI32_MAX;

				long val = strtol(pArgs[1].data(), nullptr, 0);

				CPU_OP * ops = s_text.Alloc<CPU_OP>(2);
				ops[0].WORD = CPU_LUI;
				ops[1].WORD = CPU_ORI;

				ops[0].I.rt = 1;
				ops[1].I.rs = 1;
				ops[1].I.rt = p1.Value;

				ops[0].I.u = val >> 16;
				ops[1].I.u = val & 0xFFFF;

				// Was this maybe supposed to be a label?
				if (val == 0 && pArgs[1][0] != '0')
				{
					References.push_back(ProcNode(pArgs[1], reinterpret_cast<char*>(ops)-s_text.Begin(), R_MIPS_HI16));
					References.push_back(ProcNode(pArgs[1], reinterpret_cast<char*>(ops + 1) - s_text.Begin(), R_MIPS_LO16));
				}

				return 2;
			}	break;
			#pragma endregion
			case(0x02) :	// li
			#pragma region LI
			{
				iNode p1 = MatchNode(pArgs[0]);
				if (p1.t8 != NODE_TYPE_REGISTER)
					return _UI32_MAX;

				long val = strtol(pArgs[1].data(), nullptr, 0);

				if (val >= _I16_MIN || val <= _UI16_MAX)
				{
					CPU_OP * ops = s_text.Alloc<CPU_OP>();
					ops[0].WORD = CPU_ADDI;
					ops[0].I.rt = p1.Value;
					if (val < 0)
						ops[0].I.i = val;
					else
						ops[0].I.u = val;
				}
				else
				{
					CPU_OP * ops = s_text.Alloc<CPU_OP>(2);
					ops[0].WORD = CPU_LUI;
					ops[1].WORD = CPU_ORI;

					ops[0].I.rt = 1;
					ops[1].I.rs = 1;
					ops[1].I.rt = p1.Value;

					ops[0].I.u = val >> 16;
					ops[1].I.u = val & 0xFFFF;
				}

				return 2;

			}	break;
			#pragma endregion
			case (0x03) :	// push
			#pragma region PUSH
			{
				iNode p1 = MatchNode(pArgs[0]);
				if (p1.t8 != NODE_TYPE_REGISTER)
					return _UI32_MAX;

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

				ops[2].I.rt = p1.Value;	// sw $VAL,($sp)
				ops[2].I.rs = 0x1D;

				return 1;

			}	break;
			#pragma endregion
			case (0x04) :	// pop
			#pragma region POP
			{
				iNode p1 = MatchNode(pArgs[0]);
				if (p1.t8 != NODE_TYPE_REGISTER)
					return _UI32_MAX;

				CPU_OP * ops = s_text.Alloc<CPU_OP>(3);
				ops[0].WORD = CPU_LUI;
				ops[1].WORD = CPU_LW;
				ops[2].WORD = 0;

				ops[0].I.rt = 0x01;		// li $r1, 4
				ops[0].I.u = 4;

				ops[1].I.rt = p1.Value;	// lw $VAL,($sp)
				ops[1].I.rs = 0x1D;

				ops[2].R.op = CPU_FUNC;	// subu $sp,$sp,$r1
				ops[2].R.funct = CPU_FUNC_ADDU;
				ops[2].R.rd = 0x1D;
				ops[2].R.rs = 0x1D;
				ops[2].R.rt = 0x01;

				return 1;

			}	break;
			#pragma endregion
			}
		}

		bool Builder::GenELF(char const * const pFile)
		{
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

				o_rodata_hdr->sh_name		= AddString(".symtab");
				o_rodata_hdr->sh_type		= SHT_SYMTAB;
				o_rodata_hdr->sh_offset		= o_symtab.offset;
				o_rodata_hdr->sh_size		= s_symtab.Offset();
				o_symtab_hdr->sh_addralign	= 0x04;
				o_symtab_hdr->sh_entsize = sizeof(Elf32_Sym);
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
				o_text = { ELF.Offset(), &ELF };
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
				o_text_hdr->sh_addralign = 0x04;
				o_rela_hdr->sh_link = si_symtab;
				o_rela_hdr->sh_info = si_text;
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
				o_text_hdr->sh_type		= SHT_STRTAB;
				o_text_hdr->sh_flags	= 0;

				// data
				o_strtab = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_strtab.Offset()), s_strtab.Begin(), s_strtab.Offset());

				// shdr				
				o_strtab_hdr->sh_offset		= o_strtab.offset;
				o_strtab_hdr->sh_size		= s_strtab.Offset();
				o_strtab_hdr->sh_addralign	= 0x1;

				o_ehdr->e_shstrndx = si_strtab;
			}
		#pragma endregion

		#pragma region SHDR
			Offset<Elf32_Shdr>	o_shdr;
			{
				o_shdr = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_hdr.Offset()), s_hdr.Begin(), s_hdr.Offset());

				o_ehdr->e_shoff = o_shdr.offset;
			}
		#pragma endregion

			std::ofstream fout(pFile, std::ios::out | std::ios::binary | std::ios::trunc);
			if (!fout.is_open())
			{
				std::cout << "ERROR : Unable to write to file " << pFile << std::endl;
				return false;
			}

			fout.write(ELF.Begin(), ELF.Offset());
			fout.close();
		}

		Elf32_Off Builder::GenSymtab(Elf32_Half const si_text, Elf32_Half const si_data, Elf32_Half const si_rodata, Elf32_Half const si_bss)
		{
			// Mark Global Symbols
			for (unsigned __int32 l = 0; l < Labels.size(); l++)
			{
				for (unsigned __int32 g = 0; g < Globals.size(); g++)	// Is this a global label
				{
					if (Labels[l].ID == Globals[g].ID)
					{
						Globals[l].i.Value = l;		// Reference label
						Labels[l].i.p1 = 1;			// Denotes thet this is a "global" label	
						break;
					}
				}
			}

			// Detect Undefined Labels
			for (unsigned __int32 r = 0; r < References.size(); r++)
			{
				bool defined = false;
				for (unsigned __int32 l = 0; l < Labels.size(); l++)
				{
					if (Labels[l].ID == References[r].ID)
					{
						defined = true;
						break;
					}
				}

				if (!defined)
					AddLabel(ProcNode(References[r].ID, _UI32_MAX, NODE_TYPE_NONE));
			}

			// Add Local and Undefined Labels
			for (unsigned __int32 l = 0; l < Labels.size(); l++)
			{
				if (!Labels[l].i.p1)
				{
					Elf32_Sym* pSym = s_symtab.Alloc<Elf32_Sym>();
					pSym->st_name	= AddString(Labels[l].ID.c_str());
					pSym->st_value	= Labels[l].i.Value;
					pSym->st_size	= 0;
					pSym->st_other	= 0;

					switch (Labels[l].i.t8)
					{
					case(NODE_TYPE_OFFSET_T) :
						pSym->st_shndx = si_text;
						pSym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_FUNC);
						break;
					case(NODE_TYPE_OFFSET_D) :
						pSym->st_shndx = si_data;
						pSym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
						break;
					case(NODE_TYPE_OFFSET_R) :
						pSym->st_shndx = si_rodata;
						pSym->st_info = ELF32_ST_INFO(STB_LOCAL, STT_OBJECT);
						break;
					case(NODE_TYPE_OFFSET_B) :
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

			Elf32_Off o_globals = 0;
			if (Globals.size())
			{
				// Add Local and Undefined Labels
				for (unsigned __int32 l = 0; l < Labels.size(); l++)
				{
					if (!Labels[l].i.p1)
					{
						Elf32_Sym* pSym = s_symtab.Alloc<Elf32_Sym>();
						pSym->st_name = AddString(Labels[l].ID.c_str());
						pSym->st_value = Labels[l].i.Value;
						pSym->st_size = 0;
						pSym->st_other = 0;

						switch (Labels[l].i.t8)
						{
						case(NODE_TYPE_OFFSET_T) :
							pSym->st_shndx = si_text;
							pSym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
							break;
						case(NODE_TYPE_OFFSET_D) :
							pSym->st_shndx = si_data;
							pSym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT);
							break;
						case(NODE_TYPE_OFFSET_R) :
							pSym->st_shndx = si_rodata;
							pSym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT);
							break;
						case(NODE_TYPE_OFFSET_B) :
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
						p_rela->r_offset = References[r].i.Value;
						p_rela->r_info = ELF32_R_INFO(s, References[r].i.t8);
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
			return (strcpy(s_strtab.Alloc(strlen(pString) + 1), pString) - s_strtab.Begin());
		}
	}
}