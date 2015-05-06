#pragma once

// Via ELF Spec 1.1

#define EI_NINDENT	16	// Size of e_ident[]
#define EI_MAG0		0	// File identification
#define EI_MAG1		1	// File identification
#define EI_MAG2		2	// File identification
#define EI_MAG3		3	// File identification
#define EI_CLASS	4	// File class : 1 = 32bit, 2 = 64bit
#define EI_DATA		5	// Data encoding : 1 = little endian, 2 = big endian
#define EI_VERSION	6	// File version : 1 for original elf
#define EI_PAD		7	// Start of padding bytes

#define ELFMAG0		0x7f	// e_ident[EI_MAG0]
#define ELFMAG1		'E'		// e_ident[EI_MAG1]
#define ELFMAG2		'L'		// e_ident[EI_MAG2]
#define ELFMAG3		'F'		// e_ident[EI_MAG3]

#define ET_NONE 0	// No file type
#define ET_REL	1	// Relocatable file
#define ET_EXEC 2	// Executable file
#define ET_DYN	3	// Shared object file
#define ET_CORE	4

#define EM_NONE 0	// No machine
#define EM_MIPS	8	// MIPS

#define EF_MIPS_NOREORDER	1	// .noreorder directive present // IGNORED
#define EF_MIPS_PIC			2	// Position independant code

#define EV_NONE		0
#define EV_CURRENT	1

#define SHT_NULL		0
#define SHT_PROGBITS	1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff

#define SHF_WRITE		0x1	// Section contains writable data
#define SHF_ALLOC		0x2	// Section occupies memory during process exec
#define SHF_EXECINSTR	0x4	// Section contains executable machine code

//// Symbol Table Entries

#define ELF32_ST_BIND(i)	((i) >> 4)		// Extracts bind info
#define ELF32_ST_TYPE(i)	((i)& 0 x f)	// Extracts type
#define ELF32_ST_INFO(b, t)	(((b) << 4) + ((t)& 0xf))

#define STB_LOCAL	0	// Symbol NOT visible outside its object file
#define STB_GLOBAL	1	// Symbol visible outside its object file
#define STB_WEAK	2	// Symbol visible outside its object file, lower priority than global
#define STB_LOPROC	13	// Reserved
#define STB_HIPROC	15	// Reserved

#define STT_NOTYPE	0	// Unspecified Type
#define STT_OBJECT	1	// Data object
#define STT_FUNC	2	// Function or executable code
#define STT_SECTION	3	// Associated with a section
#define STT_FILE	4	// Refers to a file
#define STT_LOPROC	13	// Reserved
#define STT_HIPROC	15	// Reserved

#define SHN_UNDEF	0		// 
#define SHN_ABS		0xfff1	// Symbol has absoule value
#define SHN_COMMON	0xfff2	// External (a.k.a. stored in RAM) global variable

//// Relocation entries

#define ELF32_R_SYM(i)		((i) >> 8)				// Extracts symbol table index
#define ELF32_R_TYPE(i)		((unsigned char) (i))	// Extracts relocation operation
#define ELF32_R_INFO(s, t)	(((s) << 8) + (unsigned char) (t))

#define R_MIPS_NONE		0	//
#define R_MIPS_16		1	// S + sign-extend(A)
#define R_MIPS_32		2	// S + A
#define R_MIPS_REL32	3	// A - EA + S
#define R_MIPS_26		4	// (((A<<2) | (P & 0xF0000000) + S) >> 2
#define R_MIPS_HI16		5	// 
#define R_MIPS_LO16		6	//
#define R_MIPS_PC16		10	// (((A<<2) | (P & 0xF0000000) + S) >> 2


namespace VGS
{
	namespace Compiler
	{

		// ELF32 Specification typedef(s)
		typedef unsigned	__int32	Elf32_Addr;
		typedef unsigned	__int16	Elf32_Half;
		typedef unsigned	__int32	Elf32_Off;
		typedef				__int32 Elf32_Sword;
		typedef unsigned	__int32 Elf32_Word;

		struct Elf32_Ehdr	// ELF header
		{
			unsigned char	e_ident[EI_NINDENT];
			Elf32_Half		e_type;			// Object file type
			Elf32_Half		e_machine;		// Machine architecture
			Elf32_Word		e_version;		// Object file version
			Elf32_Addr		e_entry;		// Memory address of the entry point, 0 if none associated
			Elf32_Off		e_phoff;		// Points to start of program header table, 0 if none
			Elf32_Off		e_shoff;		// Points to start of section header table, 0 if none
			Elf32_Word		e_flags;		// 
			Elf32_Half		e_ehsize;		// Size of this header
			Elf32_Half		e_phentsize;	// Size of a program header table item
			Elf32_Half		e_phnum;		// Number of entries in the program header table
			Elf32_Half		e_shentsize;	// Size of a section header table entry
			Elf32_Half		e_shnum;		// Number of entries in the section table
			Elf32_Half		e_shstrndx;		// Index of section headers table that contains section names
		};

		struct Elf32_Shdr	// Section Header
		{
			Elf32_Word		sh_name;		// Specifies name of section. Index into section header string table
			Elf32_Word		sh_type;		// Categorizes sections content and semantics
			Elf32_Word		sh_flags;		// Section flags
			Elf32_Addr		sh_addr;		// Address where section should appear in memory
			Elf32_Off		sh_offset;		// Offset to section in file. Otherwise 0
			Elf32_Word		sh_size;		// Size of section in bytes
			Elf32_Word		sh_link;		// Section header index link
			Elf32_Word		sh_info;		// Extra info, type dependant
			Elf32_Word		sh_addralign;	// Alignment value
			Elf32_Word		sh_entsize;		// Size of an entry if section holds table of entries
		};

		struct Elf32_Sym	// Symbol Table Entry
		{
			Elf32_Word		st_name;		// Specifies name of symbol table entry. Index into string table
			Elf32_Addr		st_value;		// Gives the value associated with the symbol
			Elf32_Word		st_size;		// Size of symbol
			unsigned char	st_info;			// Holds st_bind, st_type
			unsigned char	st_other;		// Always 0
			Elf32_Half		st_shndx;		// Section header index
		};

		struct Elf32_Rel		// Relocation Entry
		{
			Elf32_Addr		r_offset;		// The location of where to apply the relocation action
			Elf32_Word		r_info;			// Stores both symbol table index and type of relocation
		};

		struct Elf32_Rela		// Relocation Entry
		{
			Elf32_Addr		r_offset;		// The location of where to apply the relocation action
			Elf32_Word		r_info;			// Stores both symbol table index and type of relocation
			Elf32_Sword		r_addend;		// Constant used in relocation computation
		};
	}
}