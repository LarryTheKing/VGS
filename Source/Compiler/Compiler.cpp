#include "Compiler.h"

#include <iostream>
#include <fstream>
#include <string>

#include "..\CPU_OP.h"

namespace VGS
{
	namespace Compiler
	{
		Compiler::Compiler()
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

		bool Compiler::Compile(char * const fileName)
		{
			Reset();
			if (!GenLitTree(fileName))
			{
				std::cout << "ERROR : Failed to create litterature tree\n";
				return false;
			}

			return true;
		}

		void Compiler::LoadLanguage(char const * const fileName)
		{
			std::ifstream ifile(fileName);
			std::cout << "Opening Language File : \"" << fileName << "\"\n";
			if (!ifile.is_open())
			{
				std::cout << "ERROR : Failed to open Language File" << std::endl;
			}

			ProcNode node;

			while ((ifile >> node.ID >> std::hex >> node.Value >> std::hex >> node.Type))
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
			std::cout << "\nOpening file \"" << fileName << "\"\n";
			if (!ifile.is_open())
			{
				std::cout << "ERROR : Failed to open file." << std::endl;
				return false;
			}

			// Read each line from file and generate litterature tree
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

			return true;
			// TODO : Allow include files
		}
	}
}
