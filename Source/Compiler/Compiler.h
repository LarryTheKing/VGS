#pragma once

#include "Node.h"
#include "Stack.h"
#include <vector>

namespace VGS
{
	namespace Compiler
	{
		class Compiler
		{
		private:
			std::vector<ProcNode>		Language;
			std::vector<std::string>	Litterature;
			
		public:
			Compiler();
			~Compiler();

			bool Compile(char* const);

		private:

			void	LoadLanguage(char const * const);
			void	AddProcNode(char * const);

			bool	GenLitNode(std::string const);
			bool	GenLitTree(char* const);
			
			void	Reset(void);
		};
	}
}

