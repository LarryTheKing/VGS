#include <fstream>
#include <string>
#include <cstdlib>
#include <iostream>

#include "Loader.h"
#include "Compiler\ELF.h"

namespace VGS
{
	using namespace Compiler;
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
		if (e_hdr.e_type != ET_EXEC ||
			e_hdr.e_machine != EM_MIPS ||
			e_hdr.e_flags != EF_MIPS_NOREORDER)
		{
			std::cout << "ERROR : Incompatible type" << std::endl;
			return nullptr;
		}

		// Okay, this file checks out. Load it!
		char * pData = reinterpret_cast<char*>(malloc(fileSize));
		fin.seekg(0, std::ios_base::beg);
		fin.read(pData, fileSize);
		fin.close();

		std::cout << ">> Is compatible ELF file" << std::endl;

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

	bool Loader::LoadELF(System * const pSys, char const * const pFile)
	{
		std::cout << std::endl << "Loading ELF file \"" << pFile << "\"" << std::endl;
		char * const pData = VerifyELF(pFile);
		if (!pData)
		{
			std::cout << "ERROR : Unable to verify " << pFile << std::endl;
			return false;
		}

		Elf32_Shdr * pShdrText = GetShdrByName(pData, ".text");
		Elf32_Shdr * pShdrData = GetShdrByName(pData, ".data");

		if (!pShdrText || !pShdrText->sh_size){
			std::cout << "ERROR : No ROM found " << std::endl;
		}
		else {
			// Clear old ROM and copy in new ROM
			pSys->vROM.~MEM();
			new (&pSys->vROM)MEM(pShdrText->sh_size);
			memcpy(pSys->vROM.pData, pData + pShdrText->sh_offset, pShdrText->sh_size);
			std::cout << ">> Found " << pShdrText->sh_size << " bytes of ROM" << std::endl;
		}

		if (pShdrData)
		{
			if (pShdrData->sh_size > pSys->vRAM.size)
			{
				std::cout << "ERROR : Not enough RAM, requires " << pShdrData->sh_size << "bytes" << std::endl;
				return false;
			}

			memcpy(pSys->vRAM.pData, pData + pShdrData->sh_offset, pShdrData->sh_size);
			std::cout << ">> Found " << pShdrData->sh_size << " bytes of RAM" << std::endl;
		}

		std::cout << std::endl << "Load successful!" << std::endl;

		return true;
	}
}