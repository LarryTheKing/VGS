#include "Linker.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "..\CPU_OP.h"

namespace VGS
{
	namespace Compiler
	{
		Linker::Linker(void)
			: s_text(128), s_data(128), s_strtab(128), s_symtab(128), s_rela(128)
		{}

		Linker::~Linker(void)
		{
			Reset();
		}

		void Linker::Reset(void)
		{
			s_text.Reset();
			s_data.Reset();
			s_strtab.Reset();
			s_symtab.Reset();
			s_rela.Reset();
		}

		struct ObjDef
		{
			unsigned __int32 oText;
			unsigned __int32 oData;
			unsigned __int32 oRodata;
			unsigned __int32 oBss;
			unsigned __int32 oStrtab;
			unsigned __int32 oSymtab;
			unsigned __int32 oRela;
		};

		bool Linker::Link(char const * const * fin, unsigned __int32 count, char const * const fout)
		{
			std::cout << "Linking " << count << " file(s) :" << std::endl << std::endl;

			for (unsigned __int32 i = 0; i < count; i++)
			{
				std::cout << "Linking file : \"" << fin[i] << '\"' << std::endl;
				if (!AddObject(fin[i]))
				{
					std::cout << "ERROR : Failed to add object\n" << std::endl;
					return false;
				}
			}

			return true;
		}

		/**
		* Loads an ELF object and verifies that it is compatible with VGS
		*
		* @param pFile	The pointer to the file name
		* @return	Returns a pointer to a malloc() block of memory that
		*			contains the object, or nullptr on error
		* @requires pFile is a pointer to a null terminated c-string
		*			The client calls free() on the returned value when done
		*/
		char * VerifyELF(const char * const pFile)
		{
			std::ifstream fin(pFile, std::ios::in | std::ios::binary | std::ios::ate);
			if (!fin.is_open())
			{
				std::cout << "ERROR : Unable to open file " << pFile << std::endl;
				return nullptr;
			}

			// Verify that this could be an ELF file
			size_t fileSize = static_cast<size_t>(fin.tellg());
			if (fileSize < sizeof(Elf32_Ehdr))
			{
				std::cout << "ERROR : Not an ELF object" << std::endl;
				return nullptr;
			}

			// Load and check header
			Elf32_Ehdr e_hdr;
			fin.seekg(0, std::ios_base::beg);
			fin.read(reinterpret_cast<char*>(&e_hdr), sizeof(Elf32_Ehdr));

			// Check magic number
			if (e_hdr.e_ident[EI_MAG0] != ELFMAG0 ||
				e_hdr.e_ident[EI_MAG1] != ELFMAG1 ||
				e_hdr.e_ident[EI_MAG2] != ELFMAG2 ||
				e_hdr.e_ident[EI_MAG3] != ELFMAG3)
			{
				std::cout << "ERROR : Not an ELF object" << std::endl;
				return nullptr;
			}

			// Check encoding
			if (e_hdr.e_ident[EI_CLASS]		!= 1 ||
				e_hdr.e_ident[EI_DATA]		!= 1 ||
				e_hdr.e_ident[EI_VERSION]	!= 1)
			{
				std::cout << "ERROR : Unrecognized encoding" << std::endl;
				return nullptr;
			}

			// Check type
			if (e_hdr.e_type != ET_REL ||
				e_hdr.e_machine != EM_MIPS ||
				e_hdr.e_flags != EF_MIPS_PIC )
			{
				std::cout << "ERROR : Incompatible type" << std::endl;
				return nullptr;
			}

			// Okay, this file checks out. Load it!
			char * pData = reinterpret_cast<char*>(malloc(fileSize));
			fin.seekg(0, std::ios_base::beg);
			fin.read(pData, fileSize);
			fin.close();

			std::cout << ">> Is compatible ELF file" << std::endl << std::endl;

			// Return pointer to loaded content
			return pData;
		}

		Elf32_Shdr * GetShdrByIndex(char * const pData, unsigned __int32 const index)
		{
			// Finds where the section headers are stored, returns the index'th one
			return reinterpret_cast<Elf32_Shdr * const>(reinterpret_cast<Elf32_Ehdr * const>(pData)->e_shoff + pData) + index;
		}

		Elf32_Shdr * GetShdrByName(char * const pData, char const * const pName)
		{
			// Find the section header that describes where section names are stored
			Elf32_Shdr * pShdr = GetShdrByIndex(pData, reinterpret_cast<const Elf32_Ehdr*>(pData)->e_shstrndx);
			// Find these section names
			const char * const pStr = pData + pShdr->sh_offset;

			// Check for a matching section name
			const unsigned __int32 Shnum = reinterpret_cast<Elf32_Ehdr*>(pData)->e_shnum;
			for (unsigned __int32 i = 0; i < Shnum; i++)
			{
				// Get the section at index i
				pShdr = GetShdrByIndex(pData, i);
				// Compare section name and pName
				if (strcmp(pStr + pShdr->sh_name, pName) == 0)
					return pShdr;	// We found a match, return the section
			}

			return nullptr;
		}

		char const * const GetShdrName(char * const pData, unsigned __int32 index)
		{
			// Find the section header that describes where section names are stored
			Elf32_Shdr * pShdr = GetShdrByIndex(pData, reinterpret_cast<const Elf32_Ehdr*>(pData)->e_shstrndx);
			// Find these section names
			const char * const pStr = pData + pShdr->sh_offset;

			// Get the section at index
			pShdr = GetShdrByIndex(pData, index);
			// Return the name
			return pStr + pShdr->sh_name;
		}
	
		bool Linker::AddSections(char * const pData)
		{
			// How many sections are there
			const unsigned __int32 Shnum = reinterpret_cast<Elf32_Ehdr*>(pData)->e_shnum;

			Elf32_Shdr * pShdr;

			// Print message
			std::cout << "Allocating sections..." << std::endl;

			// Add allocatable sections
			for (unsigned __int32 i = 0; i < Shnum; i++)
			{
				pShdr = GetShdrByIndex(pData, i);
				// Is this allocatable
				if ((pShdr->sh_flags & SHF_ALLOC) == SHF_ALLOC)
				{
					// Does this go in ROM or RAM
					DynamicStackAlloc * pStack = ((pShdr->sh_flags & SHF_WRITE) == SHF_WRITE ? &s_data : &s_text);

					// Align stack to the section
					if (pShdr->sh_addralign)
						pStack->Align(pShdr->sh_addralign);

					// Update section address
					pShdr->sh_addr = pStack->Offset();

					// Allocate enough space to store the data
					char * const pSpace = pStack->Alloc(pShdr->sh_size);
					if (!pSpace){	// Check that we actually allocated space
						std::cout << "ERROR : \"" << GetShdrName(pData, i)
							<< "\" Out of memory; Unable to allocate " << pShdr->sh_size << " bytes" << std::endl;
						return false;
					}

					// Add the section data
					if(pShdr->sh_type == SHT_PROGBITS)	// Is there any data here
						memcpy(pSpace, pData + pShdr->sh_offset, pShdr->sh_size);	// Copy data
					else
						memset(pSpace, 0x00, pShdr->sh_size);	// Zero space

					// Print message
					std::cout << ">> " << GetShdrName(pData, i) << "\t: " << pShdr->sh_size << "\tbytes\t@ 0x" 
						<< std::hex << pShdr->sh_addr <<std::dec<<std::endl;
				}
			}

			std::cout << std::endl;

			return true;
		}

		bool Linker::ParseSymTab(Elf32_Shdr * const pShdr, char * const pData)
		{
			// Where are strings stored in pData
			Elf32_Shdr * pShdrStrtab = GetShdrByIndex(pData, reinterpret_cast<const Elf32_Ehdr*>(pData)->e_shstrndx);

			// Where are the symbols actually stored
			Elf32_Sym * pSymbol		= reinterpret_cast<Elf32_Sym*>(pData + pShdr->sh_offset);
			Elf32_Sym * pSymbolEnd	= reinterpret_cast<Elf32_Sym*>(pData + pShdr->sh_offset + pShdr->sh_size);

			// Where are the strings actually stored
			char * pStrtab = pData + pShdrStrtab->sh_offset;

			// Print message
			std::cout << ">> " << pStrtab + pShdr->sh_name << "\t: "
				<< pSymbol - pSymbolEnd << " References ";

			size_t nDefined = 0;
			size_t nUndefined = 0;

			// Process each symbol
			do {
				// See if this symbol is global
				if (ELF32_ST_BIND(pSymbol->st_value) == STB_GLOBAL)
				{	// Yes this symbol is defined

				}
				// See if this symbol is weak
				else if (ELF32_ST_BIND(pSymbol->st_value) == STB_WEAK)
				{

				}

			} while (++pSymbol != pSymbolEnd);

			std::cout << "( " << nUndefined << " Undefined )" << std::endl << std::endl;

			return true;
		}
		bool Linker::AddSymbols(char * const pData)
		{
			// How many sections are there
			const unsigned __int32 Shnum = reinterpret_cast<Elf32_Ehdr*>(pData)->e_shnum;

			Elf32_Shdr * pShdr;

			// Print message
			std::cout << "Adding symbols..." << std::endl;

			// Process all symbol related sections
			for (unsigned __int32 i = 0; i < Shnum; i++)
			{
				pShdr = GetShdrByIndex(pData, i);

				// Is this symtab section
				if (pShdr->sh_type == SHT_SYMTAB)
				{
					if (!ParseSymTab(pShdr, pData))
					{
						// Print error message
						std::cout << "ERROR : Unable to process symbol table" << std::endl;
						return false;
					}
				}
			}
			return true;
		}

		bool ProcessRela(Offset<CPU_OP> pAdr, Elf32_Rela const * const rela, Elf32_Sym const * const pSym, char * const pData)
		{
			const size_t symBind = ELF32_ST_BIND(pSym->st_info);

			Elf32_Shdr * pShdrParent = GetShdrByIndex(pData, pSym->st_shndx);

			const Elf32_Addr	S	= pSym->st_value;
			const Elf32_Sword	A	= rela->r_addend;
			const Elf32_Addr	EA	= pAdr.offset;
			const Elf32_Addr	P	= pShdrParent->sh_addr;

	
			switch (ELF32_R_TYPE(rela->r_info))
			{
			case R_MIPS_NONE:
				pAdr->SWORD = S;
				break;
			case R_MIPS_16:
				pAdr->I.u = (S + A) & 0xFFFF;
				break;
			case R_MIPS_32:
				pAdr->WORD = S + A;
				break;
			case R_MIPS_REL32:
				pAdr->SWORD = A - EA + S;
				break;
			case R_MIPS_26:
				pAdr->J.target = (((A << 2) | (P & 0xf0000000) + S) >> 2);
				break;
			case R_MIPS_HI16:
			{
				const size_t AHL = (rela->r_addend << 16) + (short)((rela + 1)->r_addend);
				pAdr->I.u = ((AHL + S) - (short)(AHL + S)) >> 16;
				break;
			}
			case R_MIPS_LO16:
			{
				const size_t AHL = ((rela - 1)->r_addend << 16) + (short)(rela->r_addend);
				pAdr->I.u = AHL + S;
				break;
			}
			case R_MIPS_PC16:
				pAdr->I.u = A + S - P;
				break;
			default:
				return false;
				break;
			}

			return true;
		}
		bool Linker::ParseRela(Elf32_Shdr * const pShdr, char * const pData)
		{
			Elf32_Shdr * pShdrSymtab	= GetShdrByIndex(pData, pShdr->sh_link);
			Elf32_Shdr * pShdrText		= GetShdrByIndex(pData, pShdr->sh_info);
			Elf32_Shdr * pShdrStrtab	= GetShdrByIndex(pData, reinterpret_cast<const Elf32_Ehdr*>(pData)->e_shstrndx);

			// Where is the rela data actually stored
			Elf32_Rela * pRela			= reinterpret_cast<Elf32_Rela*>(pData + pShdr->sh_offset);
			Elf32_Rela * pRelaEnd		= reinterpret_cast<Elf32_Rela*>(pData + pShdr->sh_offset + pShdr->sh_size);

			// Where are the symbols actually stored
			Elf32_Sym * pSymbols		= reinterpret_cast<Elf32_Sym*>(pData + pShdrSymtab->sh_offset);

			// Where is the .text actually stored
			Offset<CPU_OP> pText(pShdrText->sh_addr, ((pShdrText->sh_flags & SHF_WRITE) == SHF_WRITE ? &s_data : &s_text));

			// Where are the strings actually stored
			char * pStrtab = pData + pShdrStrtab->sh_offset;

			// Print message
			std::cout << ">> .rela\t: "
				<< pRelaEnd - pRela << " References ";

			size_t nDefined = 0;
			size_t nUndefined = 0;

			// Process each rela
			do {
				Offset<CPU_OP>	pFix(pText.offset + pRela->r_offset, pText.pStack);
				const unsigned __int32 sIndex = ELF32_R_SYM(pRela->r_info);
				
				// See if this symbol is defined
				if (pSymbols[sIndex].st_value != UINT_MAX)
				{	// Yes this symbol is defined
					nDefined++;
					// Process the .rela link
					if (!ProcessRela(pFix, pRela, pSymbols + sIndex, pData))
					{
						std::cout << std::endl << "ERROR : Failed to parse .rela object " << std::endl;
						return false;
					}
				}
				else // This is an undefined symbol
				{
					nUndefined++;
				}
				
			} while (++pRela != pRelaEnd);

			std::cout << "( " << nUndefined << " Undefined )" << std::endl << std::endl;

			return true;
		}
		bool Linker::LinkObject(char * const pData)
		{
			// How many sections are there
			const unsigned __int32 Shnum = reinterpret_cast<Elf32_Ehdr*>(pData)->e_shnum;

			Elf32_Shdr * pShdr;

			// Print message
			std::cout << "Linking sections..." << std::endl;

			// Process all link related sections
			for (unsigned __int32 i = 0; i < Shnum; i++)
			{
				pShdr = GetShdrByIndex(pData, i);

				// Is this .rela section
				if (pShdr->sh_type == SHT_RELA)
				{
					if (!ParseRela(pShdr, pData))
					{
						// Print error message
						std::cout << "ERROR : Unable to process .rela section" << std::endl;
						return false;
					}
				}
			}
			return true;
		}

		bool Linker::AddObject(const char * const pFile)
		{
			char * const pData = VerifyELF(pFile);
			if (!pData)
			{
				std::cout << "ERROR : Unable to verify " << pFile << std::endl;
				return false;
			}

			if (!AddSections(pData))
			{
				std::cout << "ERROR : Unable to add sections" << std::endl;
				return false;
			}

			if (!AddSymbols(pData))
			{
				std::cout << "ERROR : Unable to add symbols" << std::endl;
				return false;
			}

			if (!LinkObject(pData))
			{
				std::cout << "ERROR : Unable to link object" << std::endl;
				return false;
			}
			
				return true;
		}

		Elf32_Word Linker::AddString(char const * const pString)
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