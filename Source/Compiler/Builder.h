#pragma once 
#include <vector>
#include <initializer_list>

#include "ELF.h"
#include "Stack.h"
#include "..\CPU_OP.h"
#include "Offset.h"

namespace VGS
{
	namespace Compiler
	{
		struct Semantic;
		struct Expression;
		struct Global;
		struct Label;
		struct Reference;

		class Builder
		{
		private:
			std::vector<Semantic>		Language;
			std::vector<Expression>		Source;

			std::vector<Label>			Labels;
			std::vector<Reference>		References;
			std::vector<Global>			Globals;

			DynamicStackAlloc	s_text;		// .text
			DynamicStackAlloc	s_data;		// .data
			DynamicStackAlloc	s_rodata;	// .rodata
			unsigned __int32	s_bss;		// .bss

			DynamicStackAlloc	s_strtab;	// .strtab
			DynamicStackAlloc	s_symtab;	// .symtab
			DynamicStackAlloc	s_rela;		// .rela.text

		public:
			Builder(void);
			~Builder(void);

			bool	Build(char const * const, char const * const);
			void	Reset(void);

		private:
			void	LoadLanguage(void);
			
			bool	GenSource(char const * const);
			bool	GenSourceNodes(std::string const, const unsigned __int32);

			Semantic const * 	MatchNode(const std::string &) const;
			bool				AddGlobal(const std::string);
			bool				AddLabel(Label const);

			unsigned __int32	GenProgTree(void);
			unsigned __int32	GenText		(Expression const * );
			unsigned __int32	GenTextOp	(Offset<CPU_OP>, Expression const *, unsigned __int32);
			unsigned __int32	GenTextOpS	(Expression const * const, unsigned __int32);
			unsigned __int32	GenData		(Expression const *, Expression const * const, unsigned __int32);
			unsigned __int32	GenBss		(Expression const *, Expression const * const);

			Elf32_Word AddString(char const * const);

			bool		GenELF(char const * const);
			Elf32_Off	GenSymtab(Elf32_Half const, Elf32_Half const, Elf32_Half const, Elf32_Half const);
			void		GenRela(void);
		};

#define SEMANTIC_TYPE_UNDEFINED	_UI32_MAX
#define SEMANTIC_TYPE_REG		0x01	// Defines a register
#define SEMANTIC_TYPE_ADDRESS	0x02	// Defines an address
#define SEMANTIC_TYPE_OP		0x10	// Defines an opcode
#define SEMANTIC_TYPE_OPS		0x20	// Defines a special opcode
		// A semantic is any defined term and its meaning or values
		struct Semantic
		{		
			std::string						ID;		// The textual identity of this semantic
			unsigned __int32				Type;	// The type this semantic describes
			std::vector<unsigned __int32>	Values;	// All of the values, in order, of this semantic

			Semantic(std::string const id, unsigned __int32 const type = SEMANTIC_TYPE_UNDEFINED)
				: ID(id), Type(type) {}
			Semantic(std::string const id, unsigned __int32 const type, std::initializer_list<unsigned __int32> values)
				: ID(id), Type(type), Values(values) {}
		};

		// An expression is any string that exists on its own in an assembly languge source file
		struct Expression
		{
			std::string			Text;		// The actual text of this expression
			unsigned __int32	Line;		// The line on which this expression was encountered

			Expression(std::string const text, unsigned __int32 const line)
				: Text(text), Line(line) {};
		};

		// A Global corresponds to a label that sticks around for the duration of the linking phase
#define GLOBAL_UNDEFINED _UI32_MAX
		struct Global
		{
			std::string			ID;		// The idenitifying name of this global
			unsigned __int32	iLabel;	// The index of the label which this global refers to

			Global(std::string const id, unsigned __int32 const labelIndex = GLOBAL_UNDEFINED)
				: ID(id), iLabel(labelIndex) {};
		};

		// A Label corresponds to a specific location (offset) in memory
#define LABEL_UNDEFINED			_UI32_MAX
#define LABEL_TYPE_NONE			0x0
#define LABEL_TYPE_GLOBAL		0x100
#define LABEL_TYPE_BANK_MASK	0xFF
#define LABEL_TYPE_BANK_T		0x01	// .text
#define LABEL_TYPE_BANK_D		0x02	// .data
#define LABEL_TYPE_BANK_R		0x03	// .rodata
#define LABEL_TYPE_BANK_B		0x04	// .bss

		struct Label
		{
			std::string			ID;		// The identifying name of this label
			unsigned __int32	Offset;	// The offset associated with this label
			unsigned __int32	Type;	// The descriptive type of this label,
										//  tells us which memory space the label is in

			Label(std::string const id, unsigned __int32 const offset = LABEL_UNDEFINED, unsigned __int32 type = LABEL_TYPE_NONE)
				: ID(id), Offset(offset), Type(type) {}

			bool IsDefined(void) const { return Offset != LABEL_UNDEFINED; }
			bool IsGlobal(void) const { return (Type & LABEL_TYPE_GLOBAL) == LABEL_TYPE_GLOBAL; }
			unsigned __int32 const GetBank(void) const { return Type & LABEL_TYPE_BANK_MASK;  }

			void SetGlobal(bool const value = true)
			{
				if (value)
					Type |= LABEL_TYPE_GLOBAL;
				else
					Type &= ~LABEL_TYPE_GLOBAL;
			}
		};
#define REF_UNDEFINED			_UI32_MAX
		struct Reference
		{
			std::string			ID;		// The identifying name which we are tring to reference
			unsigned __int32	Offset;	// The offset of the opcode which is doing the referencing
			unsigned __int32	Type;	// The type of reference replacement

			Reference(std::string const id, unsigned __int32 const offset = REF_UNDEFINED, unsigned __int32 type = R_MIPS_NONE)
				: ID(id), Offset(offset), Type(type) {};
		};
	}
}