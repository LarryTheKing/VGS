#include "Linker.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

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
			std::cout << "\Linking " << count << " file(s) :" << std::endl;

			for (int i = 0; i < count; i++)
			{
				std::cout << "File :" << fin[i] << std::endl;
				if (!AddObject(fin[i]))
				{
					std::cout << "ERROR : Failed to add object\n" << std::endl;
					return false;
				}
			}
		}

		/**
		*	Loads and ELF object and verifies that it is compatible with VGS
		*
		*	@param pFile	The pointer to the file name
		*	@return			Returns a pointer to a malloc() block of memory that
		*					contains the object, or nullptr on error
		*	@requires		pFile is a pointer to a null terminated c-string
		*					The client calls free() on the returned value when done
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
			unsigned __int32 fileSize = fin.tellg();
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

		bool Linker::AddObject(const char * const pFile)
		{
			char * const pData = VerifyELF(pFile);
			if (!pData)
			{
				std::cout << "ERROR : Unable to verify " << pFile << std::endl;
				return false;
			}


		}
	}
}