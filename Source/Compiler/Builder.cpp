#include "Builder.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "..\CPU_OP.h"

namespace VGS
{
	namespace Compiler
	{

		Builder::Builder(char const * const fLang)
			: s_text(128), s_data(128), s_rodata(128), s_bss(0), s_string(128), s_symbol(128), s_rela(128)
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
		
		bool Builder::GenLitTree(char * const fileName)
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

			for (int i = 0; i < Globals.size(); i++)
			{	
				if (s == Globals[i])
					return;
			}

			Globals.push_back(s);
		}

		bool Builder::AddLabel(ProcNode const n)
		{
			if (!ValidateName(n.ID))
				return false;

			for (int i = 0; i < Labels.size(); i++)
			{
				if (n.ID == Labels[i].ID)
				{
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
						if (!AddLabel(ProcNode(cWord.substr(0, cWord.length() - 1), s_text.Offset, NODE_TYPE_OFFSET_T)))
							return false;
					else if (unsigned __int32 s = GenText(&Litterature[i]) != _UI32_MAX)
						i += s;
					else
						return false;
				}
				else if (mode == MODE_DATA)
				{
					if (unsigned __int32 s = GenData(&Litterature[i], &Litterature.back(), MODE_DATA) != _UI32_MAX)
						i += s;
					else
						return false;
				}
				else if (mode == MODE_BSS)
				{
					if (unsigned __int32 s = GenBss(&Litterature[i], &Litterature.back()) != _UI32_MAX)
						i += s;
					else
						return false;
				}
			}
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
				if (pArgs < pLast)		// Check for end of litearture
					return _UI32_MAX;

				//  Align to data sizes
				if (pArgs[1] == ".half")
				{
					if (!pStack->Align(2))
					{
						pStack->Resize(pStack->Capacity() * 2);
						pStack->Align(2);
					}
				}
				else if (pArgs[1] == ".word" || pArgs[1] == ".space")
				{
					if (!pStack->Align(4))
					{
						pStack->Resize(pStack->Capacity() * 2);
						pStack->Align(4);
					}
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
				{
					pStack->Resize(pStack->Capacity() * 2);
					pStack->Align(2);
				}

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
				{
					pStack->Resize(pStack->Capacity() * 2);
					pStack->Align(4);
				}

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

				for (int i = 0; i < val.length(); i++)
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
				if (pArgs < pLast)		// Check for end of litearture
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
				return -1;


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
						References.push_back(ProcNode(pArgs[nArgs], reinterpret_cast<char*>(op)-s_text.Begin(), (NODE_TYPE_OFFSET << 8) || NODE_TYPE_IMMEDIATE));
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
						References.push_back(ProcNode(pArgs[nArgs], reinterpret_cast<char*>(op)-s_text.Begin(), NODE_TYPE_TARGET));
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

					// Was this maybe supposed to be a label?
					if (op->I.i == 0 && pArgs[nArgs][0] != '0')
					{
						References.push_back(ProcNode(pArgs[nArgs].substr(0, paren), reinterpret_cast<char*>(op)-s_text.Begin(), NODE_TYPE_TARGET));
					}

					if (paren != std::string::npos)
					{
						std::string regArg = pArgs[nArgs].substr(paren + 1, std::string::npos);
						regArg.pop_back();	// Should remove ')'
						iNode regNode = MatchNode(regArg);
						if (regNode.t8 != NODE_TYPE_REGISTER)
							return -1;

						op->I.rs = regNode.Value;
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
					return -1;

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
					References.push_back(ProcNode(pArgs[1], reinterpret_cast<char*>(ops)-s_text.Begin(), (NODE_TYPE_UPPER << 8) | NODE_TYPE_IMMEDIATE));
					References.push_back(ProcNode(pArgs[1], reinterpret_cast<char*>(ops + 1) - s_text.Begin(), (NODE_TYPE_LOWER << 8) | NODE_TYPE_IMMEDIATE));
				}

				return 2;
			}	break;
			#pragma endregion
			case(0x02) :	// li
			#pragma region LI
			{
				iNode p1 = MatchNode(pArgs[0]);
				if (p1.t8 != NODE_TYPE_REGISTER)
					return -1;

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
					return -1;

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
					return -1;

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
	}
}