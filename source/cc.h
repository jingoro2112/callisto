#ifndef CALLISTO_CC_H
#define CALLISTO_CC_H
/*------------------------------------------------------------------------------*/

#include "c_linklist.h"
#include "c_str.h"
#include "c_linkhash.h"
#include "vm.h"

namespace Callisto
{

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
	C_str target;
	C_str plainTarget;
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
	C_str string;
	union // for bswap packing
	{
		uint64_t hash;
		int64_t i64; 
		double d;
	};

	uint64_t getValueHash()
	{
		Value V;
		V.type32 = type;
		V.string = &string;
		V.i64 = i64;
		return V.getHash();
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
	C_str enumName;
	bool defaultCase;
	bool nullCase;

	SwitchCase() { defaultCase = false; nullCase = false; }
};

//------------------------------------------------------------------------------
struct SwitchContext
{
	CCLinkList<SwitchCase> caseLocations;
	int breakTarget;
	int tableLocationId;
	char switchFlags;
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
	C_str name;
	unsigned int nameHash;
	C_str unqualifiedName;
	bool member;

	C_str history;
	C_str bytecode;
	
	int baseOffset;
	CCLinkList<LinkEntry> unitSym; // function symbols that are used
	CCLinkList<C_str> parents;
	CCLinkHash<unsigned int> childAliases;

	// symbol table for finding locations in the code
	CCLinkHash<unsigned int> jumpIndexes;
	CCLinkList<CodePointer> jumpLinkSymbol; // location in the code to add the pointer to
	CCLinkList<LoopingStructure> loopTracking;

	CCLinkList<int> bracketContext;

	CCLinkList<SwitchContext> switchContexts;

	CCLinkHash<C_str> argumentTokenMapping;

	UnitEntry()
	{
		baseOffset = 0;
	}
};

//------------------------------------------------------------------------------
struct UnitNew
{
	C_str fromSpace;
	C_str unitName;
};

//------------------------------------------------------------------------------
struct Compilation
{
	C_str err;
	C_str source;
	unsigned int pos;
	unsigned int lastPos;
	static unsigned int jumpIndex;

	CCLinkList<UnitEntry> units;
	UnitEntry* currentUnit;

	CCLinkList<C_str>* space;
	CCLinkList<C_str> defaultSpace;

	CCLinkList<UnitNew> newUnitCalls;
	CCLinkHash<C_str> symbolTable;

	CCLinkHash<LinkEntry> CTokens; // table of CTokens

	CCLinkHash<int> enumCTokens;

	C_str* output;
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
	ExpressionContext( C_str& c, C_str& h, CCLinkList<LinkEntry>& s ) : bytecode(c), symbols(s), history(h) { oper.add(); }

	C_str& bytecode;
	CCLinkList<LinkEntry>& symbols;
	C_str& history;
	CCLinkList< CCLinkList<const Operation*> > oper;
};

const C_str& formatToken( CToken const& token );

void putCToken( C_str& code, CToken& CToken );
void putStringToCodeStream( C_str& code, C_str& string );

bool getToken( Compilation& comp, C_str& token, CToken& value );

}

using namespace Callisto;

#endif


