#pragma once 

#include <vector>
#include "ELF.h"
#include "Node.h"
#include "Stack.h"


namespace VGS
{
	namespace Compiler
	{
		class Builder
		{
		private:
			std::vector<ProcNode>		Language;
			std::vector<std::string>	Litterature;

			std::vector<ProcNode>		Labels;
			std::vector<ProcNode>		References;
			std::vector<ProcNode>	Globals;

			DynamicStackAlloc	s_text;		// .text data
			DynamicStackAlloc	s_data;		// .data
			DynamicStackAlloc	s_rodata;	// .rodata
			unsigned __int32	s_bss;		// .bss

			DynamicStackAlloc	s_strtab;	// .strtab
			DynamicStackAlloc	s_symtab;	// .symtab
			DynamicStackAlloc	s_rela;		// .rela.text

		public:
			Builder(char const * const);
			~Builder();

			bool	Build(char const * const);
			void	Reset(void);

		private:
			void	LoadLanguage(char const * const);
			
			bool	GenLitTree(char* const);
			bool	GenLitNode(std::string const);

			iNode	MatchNode(const std::string &);
			bool	AddGlobal(const std::string);
			bool	AddLabel(ProcNode const);

			bool	GenProgTree(void);
			unsigned __int32	GenText		(std::string const * );
			unsigned __int32	GenTextOpS	(std::string const * const, unsigned __int32);
			unsigned __int32	GenData		(std::string const *, std::string const * const, unsigned __int32);
			unsigned __int32	GenBss		(std::string const *, std::string const * const);

			Elf32_Word AddString(char const * const);

			bool		GenELF(char const * const);
			Elf32_Off	GenSymtab(Elf32_Half const, Elf32_Half const, Elf32_Half const, Elf32_Half const);
			void		GenRela(void);
		};

	}
}