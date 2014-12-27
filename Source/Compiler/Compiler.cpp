#include "Compiler.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "..\CPU_OP.h"

namespace VGS
{
	namespace Compiler
	{
		Compiler::Compiler()
			:Text(64), Data(64)
		{
			LoadLanguage("Language.txt");
		}


		Compiler::~Compiler()
		{
			Reset();
		}

		void Compiler::Reset(void)
		{
			Litterature.clear();
		}

		char * Compiler::Compile(char * const fileName, size_t & size)
		{
			std::cout << "\nCompiling : \"" << fileName << "\"\n";

			Reset();
			if (!GenLitTree(fileName))
			{
				std::cout << "ERROR : Failed to create litterature tree\n";
				return nullptr;
			}

			if (!GenProgTree())
			{
				std::cout << "ERROR : Failed to create program tree\n";
				return nullptr;
			}

			if (!Link())
			{
				std::cout << "ERROR : Failed to link program\n";
				return nullptr;
			}

			char * s = reinterpret_cast<char*>(malloc(size = Data.Cursor() - Data.Begin() + Text.Cursor() - Text.Begin()));

			memcpy(s, Text.Begin(), Text.Cursor() - Text.Begin());
			memcpy(s + (Text.Cursor() - Text.Begin()), Data.Begin(), Data.Cursor() - Data.Begin());

			std::cout << "\nCompilation successful!\n";

			return s;
		}

		void Compiler::LoadLanguage(char const * const fileName)
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

		bool Compiler::GenLitNode(std::string const sLine)
		{
			if (!sLine[0])
				return true;

			int		index = 0;
			bool	bQuote = false;

			for (size_t i = 0; i < sLine.size() + 1; i++)
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

		bool Compiler::GenLitTree(char * const fileName)
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

		iNode Compiler::MatchNode(const std::string & rString)
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

		int Compiler::GenData(std::string const * const pArgs)
		{
			if (pArgs[0] == ".byte")
			{
				long val = strtol(pArgs[1].data(), nullptr, 0);
				if (val < _I8_MIN || val > _UI8_MAX)
				{
					std::cout << "ERROR : Value " << val << " cannot be stored in a byte\n";
					return -1;
				}
				
				if (val < 0)
					*Data.Alloc<__int8>() = val;
				else
					*Data.Alloc<unsigned __int8>() = val;

				return 1;
			}
			else if (pArgs[0] == ".half")
			{
				long val = strtol(pArgs[1].data(), nullptr, 0);
				if (val < _I16_MIN || val > _UI16_MAX)
				{
					std::cout << "ERROR : Value " << val << " cannot be stored in a half\n";
					return -1;
				}

				if (val < 0)
					*Data.Alloc<__int16>() = val;
				else
					*Data.Alloc<unsigned __int16>() = val;

				return 1;
			}
			else if (pArgs[0] == ".word")
			{
				long val = strtol(pArgs[1].data(), nullptr, 0);
				if (val < _I32_MIN || val > _UI32_MAX)
				{
					std::cout << "ERROR : Value " << val << " cannot be stored in a word\n";
					return -1;
				}

				if (val < 0)
					*Data.Alloc<__int32>() = val;
				else
					*Data.Alloc<unsigned __int32>() = val;

				return 1;
			}
			else if (pArgs[0] == ".ascii" || pArgs[0] == ".asciiz")
			{
				if (pArgs[1][0] != '\"' || pArgs[1].length() < 2 || pArgs[1][pArgs[1].length() - 1] != '\"')
				{
					std::cout << "ERROR : String " << pArgs[1] << " is not valid\n";
					return -1;
				}

				std::string const val = pArgs[1].substr(1, pArgs[1].length() - 2);
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

				if (pArgs[0] == ".ascii")
					memcpy(Data.Alloc(index), buffer, index);
				else // .asciiz
					memcpy(Data.Alloc(index + 1), buffer, index + 1);

				return 1;
			}
		}

		int Compiler::GenOp(std::string const * const pArgs)
		{
			iNode baseNode = MatchNode(pArgs[0]);

			if (baseNode.t8 == NODE_TYPE_OP_S)
				return GenOpS(pArgs + 1, baseNode.Value);
			if (baseNode.t8 != NODE_TYPE_OP)
				return -1;


			CPU_OP * op = Text.Alloc<CPU_OP>();
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
						Links.push_back(ProcNode(pArgs[nArgs], reinterpret_cast<char*>(op) - Text.Begin(), (NODE_TYPE_OFFSET << 8) || NODE_TYPE_IMMEDIATE));
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
							Links.push_back(ProcNode(pArgs[nArgs], reinterpret_cast<char*>(op)-Text.Begin(), NODE_TYPE_TARGET));
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
						Links.push_back(ProcNode(pArgs[nArgs].substr(0, paren), reinterpret_cast<char*>(op)-Text.Begin(), NODE_TYPE_TARGET));
					}

					if (paren != std::string::npos)
					{
						std::string regArg = pArgs[nArgs].substr(paren+1, std::string::npos);
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

		int	 Compiler::GenOpS(std::string const * const pArgs, unsigned __int32 op)
		{
			switch (op)
			{
			case(0x01) :	// la
			{
				iNode p1 = MatchNode(pArgs[0]);
				if (p1.t8 != NODE_TYPE_REGISTER)
					return -1;

				long val = strtol(pArgs[1].data(), nullptr, 0);

				CPU_OP * ops = Text.Alloc<CPU_OP>(2);
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
					Links.push_back(ProcNode(pArgs[1], reinterpret_cast<char*>(ops)-Text.Begin(), (NODE_TYPE_UPPER << 8) | NODE_TYPE_IMMEDIATE));
					Links.push_back(ProcNode(pArgs[1], reinterpret_cast<char*>(ops + 1) - Text.Begin(), (NODE_TYPE_LOWER << 8) | NODE_TYPE_IMMEDIATE));
				}

				return 2;

			}	break;
			case(0x02) :
			{
				iNode p1 = MatchNode(pArgs[0]);
				if (p1.t8 != NODE_TYPE_REGISTER)
					return -1;

				long val = strtol(pArgs[1].data(), nullptr, 0);

				if (val >= _I16_MIN || val <= _UI16_MAX)
				{
					CPU_OP * ops = Text.Alloc<CPU_OP>();
					ops[0].WORD = CPU_ADDI;
					ops[0].I.rt = p1.Value;
					if (val < 0)
						ops[0].I.i = val;
					else
						ops[0].I.u = val;
				}
				else
				{
					CPU_OP * ops = Text.Alloc<CPU_OP>(2);
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
			}
		}

#define MODE_DATA	0x02
#define MODE_TEXT	0x01

		bool Compiler::GenProgTree(void)
		{
			std::cout << "\nGenerating program tree...\n";

			unsigned __int32 mode = MODE_DATA;

			const size_t length = Litterature.size();
			for (size_t i = 0; i < length; i++)
			{
				std::string const cWord = Litterature[i];
				if (cWord == ".text")
					mode = MODE_TEXT;
				else if (cWord == ".data")
					mode = MODE_DATA;
				else if (cWord[cWord.length() - 1] == ':')
				{
					if (mode == MODE_DATA)
						Keys.push_back(ProcNode(cWord.substr(0, cWord.length() - 1), Data.Cursor() - Data.Begin(), NODE_TYPE_OFFSET_D));
					else if (mode == MODE_TEXT)
						Keys.push_back(ProcNode(cWord.substr(0, cWord.length() - 1), Text.Cursor() - Text.Begin(), NODE_TYPE_OFFSET_T));
				}
				else if (mode == MODE_DATA)
				{
					if (int s = GenData(&Litterature[i]) != -1)
						i += s;
					else
						return false;
				}
				else if (mode == MODE_TEXT)
				{
					int s = GenOp(&Litterature[i]);
					if ( s != -1)
						i += s;
					else
						return false;
				}
			}

			std::cout << ">> Program tree built\n>> Created " << Keys.size() << " keys\n";

			return true;
		}

		bool Compiler::Link(void)
		{
			std::cout << "\nLinking...\n";

			for (int l = 0; l < Links.size(); l++)
			{
				for (int k = 0; k < Keys.size(); k++)
				{
					if (Keys[k].ID == Links[l].ID)
					{
						size_t val = 0;
						if (Keys[k].i.Type == NODE_TYPE_OFFSET_T)
							val = Keys[k].i.Value | 0x80000000;
						else if (Keys[k].i.Type == NODE_TYPE_OFFSET_D)
							val = Keys[k].i.Value + Text.Cursor() - Text.Begin() | 0x80000000;

						if (Links[l].i.t8 == NODE_TYPE_TARGET)
						{
							reinterpret_cast<CPU_OP_J*>(Text.Begin() + Links[l].i.Value)->target = val >> 2;
							break;
						}
						else if (Links[l].i.t8 == NODE_TYPE_IMMEDIATE)
						{
							if (Links[l].i.p1 == NODE_TYPE_LOWER)
								reinterpret_cast<CPU_OP_I*>(Text.Begin() + Links[l].i.Value)->u = val & 0xFFFF;
							else if (Links[l].i.p1 == NODE_TYPE_UPPER)
								reinterpret_cast<CPU_OP_I*>(Text.Begin() + Links[l].i.Value)->u = val >> 16;
							else if (Links[l].i.p1 == NODE_TYPE_OFFSET)
								reinterpret_cast<CPU_OP_I*>(Text.Begin() + Links[l].i.Value)->u = (val - Links[l].i.Value) / 4;
							else
								reinterpret_cast<CPU_OP_I*>(Text.Begin() + Links[l].i.Value)->u = val;

							break;
						}
					}
				}
			}

			std::cout << ">> Made " << Links.size() << " links\n";

			return true;
		}
	}
}
