#ifndef CC_H
#define CC_H
/*------------------------------------------------------------------------------*/

#include "linklist.h"
#include "str.h"
#include "linkhash.h"
#include "vm.h"

//------------------------------------------------------------------------------
struct LoopingStructure
{
	int continueTarget;
	int breakTarget;
};

//------------------------------------------------------------------------------
struct LinkEntry
{
	unsigned int enumKey;
	Cstr target;
	Cstr plainTarget;
	unsigned int offset;
};

//------------------------------------------------------------------------------
struct CodePointer
{
	unsigned int offset;
	int targetIndex;
	int bytesForPointer;
	bool visited;
};

//------------------------------------------------------------------------------
struct CToken
{
	bool spaceBefore;
	bool spaceAfter;
	ValueType type;
	Cstr string;
	union // for bswap packing
	{
		int64_t i64; 
		double d;
	};

	unsigned int getHash()
	{
		if ( type == CTYPE_STRING )
		{
			return hash( string.c_str(), string.size() );
		}

		unsigned int u = (unsigned int)(i64 >> 32);
		if ( (u > 0) && (u < 0xFFFFFFFF) )
		{
			return u ^ (unsigned int)i64;
		}

		return (unsigned int)i64;
	}

	CToken()
	{
		type = CTYPE_NULL;
		i64 = 0;
	}
	
};

//------------------------------------------------------------------------------
struct SwitchCase
{
	CToken val;
	int jumpIndex; // where to go to get to this case
	Cstr enumName;
	bool defaultCase;
};

//------------------------------------------------------------------------------
struct SwitchContext
{
	CLinkList<SwitchCase> caseLocations;
	int breakTarget;
	int tableLocationId;
	bool hasDefault;
};

//------------------------------------------------------------------------------
enum BracketContext
{
	B_BLOCK,
	B_CASE,
	B_UNIT,
};

//------------------------------------------------------------------------------
struct UnitEntry
{
	Cstr name;
	unsigned int nameHash;
	Cstr unqualifiedName;
	bool member;

	Cstr history;
	Cstr bytecode;
	
	int baseOffset;
	CLinkList<LinkEntry> unitSym; // function symbols that are used
	CLinkList<Cstr> parents;
	CLinkHash<unsigned int> childAliases;

	// symbol table for finding locations in the code
	CLinkHash<unsigned int> jumpIndexes;
	CLinkList<CodePointer> jumpLinkSymbol; // location in the code to add the pointer to
	CLinkList<LoopingStructure> loopTracking;

	CLinkList<int> bracketContext;

	CLinkList<SwitchContext> switchContexts;

	CLinkHash<Cstr> argumentTokenMapping;

	UnitEntry()
	{
		baseOffset = 0;
	}
};

//------------------------------------------------------------------------------
struct UnitNew
{
	Cstr fromSpace;
	Cstr unitName;
};

//------------------------------------------------------------------------------
struct Compilation
{
	Cstr err;
	Cstr source;
	unsigned int pos;
	unsigned int lastPos;
	static unsigned int jumpIndex;

	CLinkList<UnitEntry> units;
	UnitEntry* currentUnit;

	CLinkList<Cstr>* space;
	CLinkList<Cstr> defaultSpace;

	CLinkList<UnitNew> newUnitCalls;
	CLinkHash<Cstr> symbolTable;

	CLinkHash<LinkEntry> CTokens; // table of CTokens

	CLinkHash<int> enumCTokens;

	Cstr* output;
};

//------------------------------------------------------------------------------
struct Operation
{
	const char* token;
	int precedence; // higher number if lower precedence
	char operation;
	bool lvalue;
	bool leftToRight;
	bool binary;
	bool postFix;
	bool matchAsCast;
};

//------------------------------------------------------------------------------
// reference:
// https://en.cppreference.com/w/cpp/language/operator_precedence
const Operation c_operations[] =
{
	{ "", 0, 0, 0, 0, 0, 0, 0 }, // placeholder operator for when the CToken can be pre-computed

	{ "==",  10, O_CompareEQ,  false, true, true, false, false },
	{ "!=",  10, O_CompareNE,  false, true, true, false, false },
	{ ">=",   9, O_CompareGE,  false, true, true, false, false },
	{ "<=",   9, O_CompareLE,  false, true, true, false, false },
	{ ">",    9, O_CompareGT,  false, true, true, false, false },
	{ "<",    9, O_CompareLT,  false, true, true, false, false },
	{ "&&",  14, O_LogicalAnd, false, true, true, false, false },
	{ "||",  15, O_LogicalOr,  false, true, true, false, false },

	{ "++",   3, O_PreIncrement, true, true, false, false, false },
	{ "++",   2, O_PostIncrement, true, true, false, true, false },

	{ "--",   3, O_PreDecrement,  true, true, false, false, false },
	{ "--",   2, O_PostDecrement, true, true, false, true, false },

	{ "copy", 2, O_CopyValue, true, true, false, false, false },

	{ "!",    3, O_LogicalNot, false, false, false, false, false },
	{ "~",    3, O_BitwiseNOT, false, false, false, false, false },

	{ "+",    6, O_BinaryAddition, false, true, true, false, false },
	{ "-",    3, O_Negate, false, false, false, false, false },

	{ "-",    6, O_BinarySubtraction, false, true, true, false, false },
	{ "*",    5, O_BinaryMultiplication, false, true, true, false, false },
	{ "/",    5, O_BinaryDivision, false, true, true, false, false },

	{ "|",   13, O_BitwiseOR, false, true, true, false, false },
	{ "&",   11, O_BitwiseAND, false, true, true, false, false },
	{ "^",   11, O_BitwiseXOR, false, true, true, false, false },

	{ ">>",   7, O_BitwiseRightShift, false, true, true, false, false },
	{ "<<",   7, O_BitwiseLeftShift, false, true, true, false, false },

	{ "+=",  16, O_AddAssign, true, true, true, false, false },
	{ "-=",  16, O_SubtractAssign, true, true, true, false, false },
	{ "%=",  16, O_ModAssign, true, true, true, false, false },
	{ "*=",  16, O_MultiplyAssign, true, true, true, false, false },
	{ "/=",  16, O_DivideAssign, true, true, true, false, false },
	{ "|=",  16, O_ORAssign, true, true, true, false, false },
	{ "&=",  16, O_ANDAssign, true, true, true, false, false },
	{ "^=",  16, O_XORAssign, true, true, true, false, false },
	{ ">>=", 16, O_RightShiftAssign, true, false, true, false, false },
	{ "<<=", 16, O_LeftShiftAssign, true, false, true, false, false },

	{ "%",    6, O_BinaryMod, false, true, true, false, false },

	{ "=",   16, O_Assign, true, false, true, false, false },

	// specially matched entries that should never match a token directly
	{ "int", 3, O_CoerceToInt, false, false, false, false, true },
	{ "float", 3, O_CoerceToFloat, false, false, false, false, true },
	{ "string", 3, O_CoerceToString, false, false, false, false, true },

	{ 0, 0, 0, 0, 0, 0, 0, 0 },
};
const int c_highestPrecedence = 17;

//------------------------------------------------------------------------------
struct ExpressionContext
{
	ExpressionContext( Cstr& c, Cstr& h, CLinkList<LinkEntry>& s ) : bytecode(c), symbols(s), history(h) { oper.add(); }

	Cstr& bytecode;
	CLinkList<LinkEntry>& symbols;
	Cstr& history;
	CLinkList< CLinkList<const Operation*> > oper;
};

const Cstr& formatToken( CToken const& token );

void putCToken( Cstr& code, CToken& CToken );
void putStringToCodeStream( Cstr& code, Cstr& string );

bool getToken( Compilation& comp, Cstr& token, CToken& value );

#endif


