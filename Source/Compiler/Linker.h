#pragma once

#include "Stack.h"
#include "ELF.h"
#include "Offset.h"

namespace VGS
{
	namespace Compiler
	{
		class Linker
		{
		private:
			DynamicStackAlloc	s_text;		// .text		a.k.a ROM
			DynamicStackAlloc	s_data;		// .data		a.k.a RAM

			DynamicStackAlloc	s_strtab;	// .strtab		For all the strings
			DynamicStackAlloc	s_symtab;	// .symtab		For global symbols
			DynamicStackAlloc	s_rela;		// .rela.text	For non-local references

		public:
			Linker(void);
			~Linker();
			void Reset(void);

			bool Link(char const * const [], unsigned __int32, char const * const);
			
		private:
			bool AddObject(const char * const);
			bool AddSections(char * const);
			bool AddSymbols(char * const);
			bool LinkObject(char * const);
			bool ParseRela(Elf32_Shdr * const, char * const);
			bool ParseSymTab(Elf32_Shdr * const, char * const);
			bool Finalize();

			Elf32_Word			AddString(char const * const);
			Offset<Elf32_Sym>	AddSymbol(char const * const);

		};
	}
}