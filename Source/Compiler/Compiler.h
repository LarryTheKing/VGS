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
			std::vector<std::string>	Vocab;
			std::vector<ProcNode>		Links;
			std::vector<ProcNode>		Keys;

			DynamicStackAlloc Text;
			DynamicStackAlloc Data;
			
		public:
			Compiler();
			~Compiler();

			char *	Compile(char* const, size_t &);

		private:

			void	LoadLanguage(char const * const);
			void	AddProcNode(char * const);
			iNode	MatchNode(const std::string &);
			bool	ProcessNode(size_t);
			
			bool	GenLitNode(std::string const);
			bool	GenLitTree(char* const);
			bool	GenProgTree();
			int		GenOp(std::string const * const);
			int		GenOpS(std::string const * const, unsigned __int32 op);
			int		GenData(std::string const * const);
			bool	Link();
			
			void	Reset(void);
		};
	}
}

