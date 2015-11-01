#include "Linker.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "..\CPU_OP.h"

#define MS_ROM	0x80000000

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
				*s_strtab.Alloc<char>() = '\0';
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

			// Link each file
			for (unsigned __int32 i = 0; i < count; i++)
			{
				std::cout << "Linking file : \"" << fin[i] << '\"' << std::endl;
				if (!AddObject(fin[i]))	// Add the contents of this object file to the program
				{
					std::cout << "ERROR : Failed to add object\n" << std::endl;
					return false;
				}
			}

			if (!Finalize())	// Finalize any remaining references
			{
				std::cout << "ERROR : Unable to finalize" << std::endl;
				return false;
			}

			if (!GenELF(fout))
			{
				std::cout << "ERROR : Failed to create ELF object\n";
				return false;
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
			if (e_hdr.e_ident[EI_CLASS] != 1 ||
				e_hdr.e_ident[EI_DATA] != 1 ||
				e_hdr.e_ident[EI_VERSION] != 1)
			{
				std::cout << "ERROR : Unrecognized encoding" << std::endl;
				return nullptr;
			}

			// Check type
			if (e_hdr.e_type != ET_REL ||
				e_hdr.e_machine != EM_MIPS ||
				e_hdr.e_flags != EF_MIPS_PIC)
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
					if (pShdr->sh_type == SHT_PROGBITS)	// Is there any data here
						memcpy(pSpace, pData + pShdr->sh_offset, pShdr->sh_size);	// Copy data
					else
						memset(pSpace, 0x00, pShdr->sh_size);	// Zero space

					// Print message
					std::cout << ">> " << GetShdrName(pData, i) << "\t: " << pShdr->sh_size << "\tbytes\t@ 0x"
						<< std::hex << pShdr->sh_addr << std::dec << std::endl;
				}
			}

			std::cout << std::endl;

			return true;
		}

		Offset<Elf32_Sym>	Linker::AddSymbol(char const * const pName)
		{
			// Check each symbol for matching text
			// Return an existing symbol if a match exists
			for (Elf32_Sym * pSym = reinterpret_cast<Elf32_Sym*>(s_symtab.Begin());
				pSym != reinterpret_cast<Elf32_Sym*>(s_symtab.Cursor()); pSym++)
			{
				// If the symbol name matches
				if (strcmp(pSym->st_name + s_strtab.Begin(), pName) == 0)
				{
					return Offset<Elf32_Sym>(pSym, &s_symtab);
				}
			}

			// Add symbol
			Elf32_Sym * pSym = s_symtab.Alloc<Elf32_Sym>();
			memset(&pSym->st_value, NULL, sizeof(Elf32_Sym) - sizeof(pSym->st_name));
			pSym->st_name = AddString(pName);
			return Offset<Elf32_Sym>(pSym, &s_symtab);
		}
		bool Linker::ParseSymTab(Elf32_Shdr * const pShdr, char * const pData)
		{
			// Where are strings stored in pData
			Elf32_Shdr * pShdrStrtab = GetShdrByIndex(pData, reinterpret_cast<const Elf32_Ehdr*>(pData)->e_shstrndx);

			// Where are the symbols actually stored
			Elf32_Sym * pSymbol = reinterpret_cast<Elf32_Sym*>(pData + pShdr->sh_offset);
			Elf32_Sym * pSymbolEnd = reinterpret_cast<Elf32_Sym*>(pData + pShdr->sh_offset + pShdr->sh_size);

			// Where are the strings actually stored
			char * pStrtab = pData + pShdrStrtab->sh_offset;

			// Print message
			std::cout << ">> " << pStrtab + pShdr->sh_name << "\t: "
				<< pSymbolEnd - pSymbol << " Symbols " << std::endl;

			size_t nGlobal = 0;
			size_t nWeak = 0;
			size_t nUndefined = 0;

			// Process each symbol
			do {

				// Is this an undefined symbol
				if (pSymbol->st_shndx == SHN_UNDEF)
				{
					// Add symbol / find existing symbol
					Offset<Elf32_Sym> oSym = AddSymbol(pStrtab + pSymbol->st_name);

					// Remember index to symbol
					pSymbol->st_value = oSym.offset / sizeof(Elf32_Sym);

					nUndefined++;
				}
				// If symbol references a section, update its value
				else if (pSymbol->st_shndx < reinterpret_cast<const Elf32_Ehdr*>(pData)->e_shnum)
				{
					// Get the corresponding section
					Elf32_Shdr * pShdr = GetShdrByIndex(pData, pSymbol->st_shndx);

					// Offset the value according to the section
					pSymbol->st_value += pShdr->sh_addr;

					// Is this section in ROM?
					if ((pShdr->sh_flags & (SHF_WRITE | SHF_ALLOC)) == (SHF_ALLOC))
					{
						pSymbol->st_value |= MS_ROM;
					}
				}

				// See if this symbol is global
				if (ELF32_ST_BIND(pSymbol->st_info) == STB_GLOBAL)
				{
					// Add symbol / find existing symbol
					Offset<Elf32_Sym> oSym = AddSymbol(pStrtab + pSymbol->st_name);

					// Check if global symbol already existed
					// If symbol is undefined 
					if (oSym->st_other)
					{
						std::cout << "ERROR : Redefinition of existing global \"" << pStrtab + pSymbol->st_name << " \"" << std::endl;
						return false;
					}

					// Add data to symbol
					oSym->st_value = pSymbol->st_value;
					oSym->st_size = pSymbol->st_size;
					oSym->st_info = pSymbol->st_info;
					oSym->st_shndx = 1;

					// Mark as global symbol
					oSym->st_other = true;

					nGlobal++;

				}
				// See if this symbol is weak
				else if (ELF32_ST_BIND(pSymbol->st_info) == STB_WEAK)
				{
					// Add symbol / find existing symbol
					Offset<Elf32_Sym> oSym = AddSymbol(pStrtab + pSymbol->st_name);

					// Add data to symbol
					oSym->st_value = pSymbol->st_value;
					oSym->st_size = pSymbol->st_size;
					oSym->st_info = pSymbol->st_info;
					oSym->st_shndx = 1;

					nWeak++;
				}

			} while (++pSymbol != pSymbolEnd);


			if (nUndefined)
				std::cout << ">>>> Undefined\t: " << nUndefined << std::endl;
			if (nGlobal)
				std::cout << ">>>> Global\t: " << nGlobal << std::endl;
			if (nWeak)
				std::cout << ">>>> Weak\t: " << nGlobal << std::endl;

			std::cout << std::endl;

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

		bool ProcessRela(Offset<CPU_OP> pAdr, Elf32_Rela const * const rela, Elf32_Sym const * const pSym, const Elf32_Addr P)
		{
			const size_t symBind = ELF32_ST_BIND(pSym->st_info);

			const Elf32_Addr	S = pSym->st_value;
			const Elf32_Sword	A = rela->r_addend;
			const Elf32_Addr	EA = pAdr.offset;

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
			Elf32_Shdr * pShdrSymtab = GetShdrByIndex(pData, pShdr->sh_link);
			Elf32_Shdr * pShdrText = GetShdrByIndex(pData, pShdr->sh_info);
			Elf32_Shdr * pShdrStrtab = GetShdrByIndex(pData, reinterpret_cast<const Elf32_Ehdr*>(pData)->e_shstrndx);

			// Where is the rela data actually stored
			Elf32_Rela * pRela = reinterpret_cast<Elf32_Rela*>(pData + pShdr->sh_offset);
			Elf32_Rela * pRelaEnd = reinterpret_cast<Elf32_Rela*>(pData + pShdr->sh_offset + pShdr->sh_size);

			// Where are the symbols actually stored
			Elf32_Sym * pSymbols = reinterpret_cast<Elf32_Sym*>(pData + pShdrSymtab->sh_offset);

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
				if (pSymbols[sIndex].st_shndx)
				{	// Yes this symbol is defined
					nDefined++;
					// Process the .rela link
					if (!ProcessRela(pFix, pRela, pSymbols + sIndex, GetShdrByIndex(pData, (pSymbols + sIndex)->st_shndx)->sh_addr))
					{
						std::cout << std::endl << "ERROR : Failed to parse .rela object " << std::endl;
						return false;
					}
				}
				else // This is an undefined symbol
				{
					nUndefined++;
					// Add this to the list of rela operations to evaluate at the end
					Offset<Elf32_Rela> oRela(s_rela.Alloc<Elf32_Rela>(), &s_rela);
					oRela->r_addend = pRela->r_addend;
					oRela->r_info = ELF32_R_INFO(pSymbols[sIndex].st_value, ELF32_R_TYPE(pRela->r_info));
					oRela->r_offset = pRela->r_offset;
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
			// Attempt to load the ELF object, verifying that it is an ELF object
			char * const pData = VerifyELF(pFile);
			if (!pData)
			{
				std::cout << "ERROR : Unable to verify " << pFile << std::endl;
				return false;
			}

			// Add each section of this object
			if (!AddSections(pData))
			{
				std::cout << "ERROR : Unable to add sections" << std::endl;
				return false;
			}

			// 
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

		bool Linker::Finalize(void)
		{
			std::cout << "Finalizing..." << std::endl;

			Elf32_Rela *	pRela		= reinterpret_cast<Elf32_Rela*>(s_rela.Begin());
			Elf32_Rela *	pRelaEnd	= reinterpret_cast<Elf32_Rela*>(s_rela.Cursor());
			Elf32_Sym *		pSymTab		= reinterpret_cast<Elf32_Sym*>(s_symtab.Begin());

			// Process each rela
			do {
				// Find the corresponding symbol
				Elf32_Sym * pSym = pSymTab + ELF32_R_SYM(pRela->r_info);

				// See if this symbol is defined
				if (pSym->st_info)
				{
					// Process the .rela link
					Offset<CPU_OP> pFix(pRela->r_offset, &s_text);
					ProcessRela(pFix, pRela, pSym, 0);
				}
			else
				{
					std::cout << "ERROR : Undefined symbol \"" << s_strtab.Begin() + pSymTab[ELF32_R_SYM(pRela->r_info)].st_name
						<< "\" : Aborting" << std::endl;
					return false;
				}
			} while (++pRela != pRelaEnd);

			return true;
		}
	
		bool Linker::GenELF(char const * const pFile)
		{
			std::cout << "\nGenerating ELF object\n";

			DynamicStackAlloc ELF(512);
			DynamicStackAlloc s_hdr(128);	// Temp storage for section headers
			s_strtab.Reset();				// We will reuse s_strtab
			*s_strtab.Alloc<char>() = 0;

			Elf32_Half cursec = 1;	// Tracks current Elf32_Shdr index, starts at 1 because UNDEF section = 0
#pragma region EHDR
			Offset<Elf32_Ehdr> o_ehdr(ELF.Offset(), &ELF);
			memset(ELF.Alloc<Elf32_Ehdr>(), 0x00, sizeof(Elf32_Ehdr));	// Zero header
			o_ehdr->e_ident[EI_MAG0] = ELFMAG0;	// Magic 0x7f
			o_ehdr->e_ident[EI_MAG1] = ELFMAG1;	// 'E'
			o_ehdr->e_ident[EI_MAG2] = ELFMAG2;	// 'L'
			o_ehdr->e_ident[EI_MAG3] = ELFMAG3;	// 'F'
			o_ehdr->e_ident[EI_CLASS] = 1;	// 32 bit
			o_ehdr->e_ident[EI_DATA] = 1;	// litle endian
			o_ehdr->e_ident[EI_VERSION] = 1;	// ELF version 1
			o_ehdr->e_type = ET_EXEC;
			o_ehdr->e_machine = EM_MIPS;
			o_ehdr->e_version = EV_CURRENT;
			o_ehdr->e_flags = EF_MIPS_NOREORDER;
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

				o_text_hdr->sh_name = AddString(".text");
				o_text_hdr->sh_type = SHT_PROGBITS;
				o_text_hdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
				o_text_hdr->sh_offset = o_text.offset;
				o_text_hdr->sh_size = s_text.Offset();
				o_text_hdr->sh_addralign = 0x04;

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

				o_data_hdr->sh_name = AddString(".data");
				o_data_hdr->sh_type = SHT_PROGBITS;
				o_data_hdr->sh_flags = SHF_WRITE | SHF_ALLOC;
				o_data_hdr->sh_offset = o_data.offset;
				o_data_hdr->sh_size = s_data.Offset();
				o_data_hdr->sh_addralign = 0x04;

				// Print progress
				std::cout << ">> .data\t: " << o_data_hdr->sh_size << " \tbytes\n";
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

				o_strtab_hdr->sh_name = AddString(".strtab");
				o_strtab_hdr->sh_type = SHT_STRTAB;
				o_strtab_hdr->sh_flags = 0;

				// data
				o_strtab = { ELF.Offset(), &ELF };
				memcpy(ELF.Alloc(s_strtab.Offset()), s_strtab.Begin(), s_strtab.Offset());

				// shdr				
				o_strtab_hdr->sh_offset = o_strtab.offset;
				o_strtab_hdr->sh_size = s_strtab.Offset();
				o_strtab_hdr->sh_addralign = 0x1;

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

	}
}