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
			std::cout << "Linking " << count << " file(s) :" << std::endl;

			for (unsigned __int32 i = 0; i < count; i++)
			{
				std::cout << "File :" << fin[i] << std::endl;
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

			// Add allocatable sections
			for (unsigned __int32 i = 0; i < Shnum; i++)
			{
				pShdr = GetShdrByIndex(pData, i);
				// Is this allocatable
				if ((pShdr->sh_flags | SHF_ALLOC) == SHF_ALLOC)
				{
					// Does this go in ROM or RAM
					DynamicStackAlloc * pStack = ((pShdr->sh_flags | SHF_WRITE) == SHF_WRITE ? &s_data : &s_text);

					// Align stack to the section
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
				}
			}

			return true;
		}

		bool Linker::LinkObject(char * const pData)
		{
			// TODO
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

			if (!LinkObject(pData))
			{
				std::cout << "ERROR : Unable to link object" << std::endl;
				return false;
			}
			
				return true;
		}

	}
}