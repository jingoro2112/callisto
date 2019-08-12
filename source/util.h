#ifndef CALLISTO_UTIL_H
#define CALLISTO_UTIL_H
/*------------------------------------------------------------------------------*/

#include "../include/callisto.h"

#include <stdlib.h>
#include <stdint.h>

#include "linkhash.hpp"
#include "linklist.hpp"
#include "hashtable.hpp"
#include "hash.hpp"
#include "str.hpp"
#include "log.hpp"
#include "array.h"

extern CLog Log;

const int c_maxUnitInheritance = 8;
extern int g_Callisto_lastErr;

//------------------------------------------------------------------------------
template<class T> class RefCountedVariable
{
public:
	T item;
	
	T* getItem() { return &item; }
	RefCountedVariable<T>* getReference() { ++reference; return this; }
	void release() { if ( --reference == 0 ) { delete this; } }
	
	RefCountedVariable() : reference(1) {}
	
private:
	int reference;
};

struct TableEntry;
struct Value;
struct MetaTableEntry;

//#define clearValue(V) { if ( (V).type != CTYPE_NULL ) { clearValueEx(V); } }
inline void clearValue( Value& Value );

//------------------------------------------------------------------------------
struct Value
{
	char type;

	union // contains only pointers, so 'ref2' can be a void* class
	{
		RefCountedVariable<Cstr>* refCstr;
		RefCountedVariable<Wstr>* refWstr;
		RefCountedVariable<CLinkHash<Value>>* refLocalSpace;
		RefCountedVariable<Carray>* refArray;
		MetaTableEntry* metaTable; // if a meta-table has been loaded (on demand only) it will be hung here
		void* ref2; // must be the size of the largest member
	};

	union // contains explicit 64-bit values, so on 32-bit machines void* is not large enough for ref1
	{
		// if this is a value, this is where it is stored
		int64_t i64; 
		double d;
		Value* v; // if this is a reference, this points to the actual value (stack only does this)
		Unit* u; // if this is a CTYPE_UNIT, this is a pointer to the Unit template, and it may have valid children
		CLinkHash<Value>::Iterator* hashIterator;
		int64_t ref1; // must be the size of the largest member
	};

	unsigned int getHash()
	{
		if ( type == CTYPE_STRING ) { return hash(refCstr->item.c_str(), refCstr->item.size()); }
		else if ( type == CTYPE_WSTRING ) { return hash(refWstr->item.c_str(), refWstr->item.size() * sizeof(wchar_t)); }
		else if ( type & BIT_CAN_HASH )	{ return hash(&ref1, 8); }
		return 0;
	}

	Value() : type(CTYPE_NULL), ref2(0), ref1(0) {}
	~Value() { clearValue(*this); }
};

//------------------------------------------------------------------------------
inline void clearValue( Value& value )
{
	if ( value.type == CTYPE_NULL )
	{
		return;
	}

	if ( value.type & BIT_PASS_BY_REF )
	{
		switch( value.type )
		{
			case CTYPE_STRING: value.refCstr->release(); break;
			case CTYPE_WSTRING: value.refWstr->release(); break;
			case CTYPE_UNIT: value.refLocalSpace->release(); break;
			case CTYPE_TABLE: value.refLocalSpace->release(); break;
			case CTYPE_ARRAY: value.refArray->release(); break;
			case CTYPE_ARRAY_ITERATOR: value.refArray->release(); break;
			case CTYPE_HASH_ITERATOR: value.refLocalSpace->release(); delete value.hashIterator; break;
			case CTYPE_STRING_ITERATOR: value.refCstr->release(); break;
			case CTYPE_WSTRING_ITERATOR: value.refWstr->release(); break;
		}
	}

	value.type = CTYPE_NULL;
	value.ref1 = 0;
	value.ref2 = 0;
}

//------------------------------------------------------------------------------
struct TableEntry
{
	Value key;
	Value value;
};

struct Unit;

//------------------------------------------------------------------------------
struct ChildUnit
{
	unsigned int key;
	Unit* child;
};

//------------------------------------------------------------------------------
// a thread of execution that has its own unit space of instantiated
// units and values keyed by label
struct Callisto_ExecutionContext
{
	int numberOfArgs;
	const Value* Args;
	void* userData;

	unsigned int pos;
	int stackPointer;
	int stackSize;
	Value* stack;

	int thread; // id of thread

	enum ThreadStatus
	{
		Runnable, // can be scheduled (was woken)
		Waiting, // cannot be scheduled, but active (was yielded)
		Running, // currently running
		Inactive, // eligible to use for calling
	};
	int status;

	Value* currentUnit;
	CHashTable<ChildUnit>* searchChildren;

	Callisto_Context* callisto;

	Callisto_ExecutionContext() { stack = 0; searchChildren = 0; currentUnit = 0; }
	~Callisto_ExecutionContext() { delete[] stack; }
};

//------------------------------------------------------------------------------
struct MetaTableEntry
{
	Callisto_CallbackFunction function;
	void* userData;
};

//------------------------------------------------------------------------------
// definition of a unit, treated as templates
struct Unit
{
	unsigned int parentList[c_maxUnitInheritance]; // what parents to run when this unit is instantiated
	int numberOfParents;
	
	unsigned int textOffset; // where in the loaded block this unit resides, if it is callisto native

	Callisto_CallbackFunction function; // what to call if this is a C function palceholder
	void* userData;
	
	CHashTable<ChildUnit> childUnits; // list of units this unit is parent to
	
	CHashTable< CHashTable<MetaTableEntry> > metaTableByType; // meta-methods for this unit
};

//------------------------------------------------------------------------------
struct Callisto_Context
{
	Cstr text; // text segment 
	Cstr err; // if an error is returned, it is loaded here

	Unit globalUnit; // represents the global space of the program (the root unit, if you will)
	Value global; // the space that holds the above unit, always a "unit" type

	CHashTable<Unit> unitList; // templates, these are code segments
	CHashTable<Callisto_ExecutionContext> threads; // independant threads operating in this space
	unsigned int threadIdGenerator;

	CHashTable<Cstr> symbols; // to reverse hashes (if loaded)

	Callisto_Context()
	{
		threadIdGenerator = 0;
		global.type = CTYPE_UNIT;
		global.u = &globalUnit;
		globalUnit.function = 0;
		globalUnit.userData = 0;
		global.refLocalSpace = new RefCountedVariable<CLinkHash<Value>>;
	}
};

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
	bool canOptimize;
	bool added;
	int offset;
	int targetIndex;
};

//------------------------------------------------------------------------------
struct SwitchCase
{
	Value val;
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
	Cstr program;
	int baseOffset;
	CLinkList<LinkEntry> unitSym; // function symbols that are used
	CLinkList<Cstr> parents;
	CLinkHash<unsigned int> childAliases;

	// symbol table for finding locations in the code
	CLinkHash<int> jumpIndexes;
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
	
	CLinkHash<LinkEntry> values; // table of values

	CLinkHash<int> enumValues;
	
	Cstr* output;
};

//------------------------------------------------------------------------------
struct Reserved
{
	Cstr token;
};

//------------------------------------------------------------------------------
struct OpcodeString
{
	char opcode;
	Cstr name;
};

//------------------------------------------------------------------------------
const Reserved c_reserved[] =
{
	{ "break" },
	{ "case" },
	{ "continue" },
	{ "copy" },
	{ "default" },
	{ "do" },
	{ "else" },
	{ "enum" },
	{ "false" },
	{ "float" },
	{ "for" },
	{ "foreach" },
	{ "if" },
	{ "int" },
	{ "new" },
	{ "null" },
	{ "NULL" },
	{ "return" },
	{ "string" },
	{ "wstring" },
	{ "switch" },
	{ "true" },
	{ "unit" },
	{ "while" },
		
	{ "" },
};

//------------------------------------------------------------------------------
enum OpcodeType
{
	O_CallFromUnitSpace = 1,
	O_CallLocalThenGlobal,
	O_CallGlobalOnly,
	
	O_Return, // end of a unit

	O_ClaimArgument, // [as label] define a label which can be referred to within a unit as a passed arg value
	O_ClaimCopyArgument, // [as label] copy the value so it is NOT a reference

	O_LoadLabel, // [label key:] push this label onto the stack

	O_Literal, // [value] push this literal value onto the stack
	O_LoadEnumValue,
	O_LiteralNull, // push a literal NULL onto stack
	O_LiteralInt8, // push a literal bool false onto stack
	O_LiteralInt16, // push a literal bool false onto stack
	O_LiteralInt32, // push a literal bool false onto stack

	O_PushBlankTable,
	O_PushBlankArray,
	
	O_PopOne, // pop exactly one value from stack
	O_PopNum, // pop next 0-255 stack entries off

	O_Assign, // move stack pointer - 1 to stack pointer - 2 and reduce stack pointer by one

	// pop 2, compare and leave true or false appropriately
	O_CompareEQ, 
	O_CompareNE, 
	O_CompareGE,
	O_CompareLE,
	O_CompareGT,
	O_CompareLT,

	// long-jump versions
	O_BZ, // pop and branch if value was any kind of FALSE
	O_BNZ, // pop and branch if value was any kind of TRUE
	O_Jump, // jump to following int

	// 8 and 16-bit relative jump versions
	O_BZ8, // pop and branch if value was any kind of FALSE
	O_BZ16, // pop and branch if value was any kind of FALSE
	O_BNZ8, // pop and branch if value was any kind of TRUE
	O_BNZ16, // pop and branch if value was any kind of TRUE
	O_Jump8, // jump to following int
	O_Jump16, // jump to following int

	O_LogicalAnd, // &&
	O_LogicalOr, // ||
	O_LogicalNot, // !

	O_PostIncrement,
	O_PostDecrement,
	O_PreIncrement,
	O_PreDecrement,
	O_Negate,

	O_MemberAccess,
	O_GlobalAccess,
	
	O_CopyValue,

	O_BinaryAddition,
	O_BinarySubtraction,
	O_BinaryMultiplication,
	O_BinaryDivision,
	O_BinaryMod,
	O_AddAssign,
	O_SubtractAssign,
	O_ModAssign,
	O_MultiplyAssign,
	O_DivideAssign,
	O_RightShiftAssign,
	O_LeftShiftAssign,

	O_BitwiseOR,
	O_BitwiseAND,
	O_BitwiseNOT,
	O_BitwiseXOR,
	O_ORAssign,
	O_ANDAssign,
	O_XORAssign,
	O_BitwiseRightShift,
	O_BitwiseLeftShift,

	O_NewUnit,
	O_LoadFromContext,

	O_AddArrayElement, // add element at index to array
	O_AddTableElement,
	O_DereferenceFromTable,
	O_PushIterator,
	O_IteratorGetNextKeyValueOrBranch,
	O_IteratorGetNextValueOrBranch,
	
	O_CoerceToInt,
	O_CoerceToString,
	O_CoerceToWideString,
	O_CoerceToFloat,

	O_Switch, // switch on the value currently on the stack with the table pointed to with the next offset

	O_LAST,
};

//------------------------------------------------------------------------------
const OpcodeString c_opcodes[] =
{
	{ O_CallFromUnitSpace, "O_CallFromUnitSpace" },
	{ O_CallLocalThenGlobal, "O_CallLocalThenGlobal" },
	{ O_CallGlobalOnly, "O_CallGlobalOnly" },
	{ O_Return, "O_Return" },
	{ O_ClaimArgument, "O_ClaimArgument" },
	{ O_ClaimCopyArgument, "O_ClaimCopyArgument" },
	{ O_LoadLabel, "O_LoadLabel" },
	{ O_Literal, "O_Literal" },
	{ O_LoadEnumValue, "O_LoadEnumValue" },
	{ O_LiteralNull, "O_LiteralNull" },
	{ O_LiteralInt8, "O_LiteralInt8" },
	{ O_LiteralInt16, "O_LiteralInt16" },
	{ O_LiteralInt32, "O_LiteralInt32" },
	{ O_PushBlankTable, "O_PushBlankTable" },
	{ O_PushBlankArray, "O_PushBlankArray" },
	{ O_PopOne, "O_PopOne" },
	{ O_PopNum, "O_PopNum" },
	{ O_Jump, "O_Jump" },
	{ O_Assign, "O_Assign" },
	{ O_CompareEQ, "O_CompareEQ" },
	{ O_CompareNE, "O_CompareNE" },
	{ O_CompareGE, "O_CompareGE" },
	{ O_CompareLE, "O_CompareLE" },
	{ O_CompareGT, "O_CompareGT" },
	{ O_CompareLT, "O_CompareLT" },
	{ O_BNZ, "O_BNZ" },
	{ O_BZ, "O_BZ" },
	{ O_LogicalAnd, "O_LogicalAnd" },
	{ O_LogicalOr, "O_LogicalOr" },
	{ O_LogicalNot, "O_LogicalNot" },
	{ O_PostIncrement, "O_PostIncrement" },
	{ O_PostDecrement, "O_PostDecrement" },
	{ O_PreIncrement, "O_PreIncrement" },
	{ O_PreDecrement, "O_PreDecrement" },
	{ O_Negate, "O_Negate" },
	{ O_MemberAccess, "O_MemberAccess" },
	{ O_GlobalAccess, "O_GlobalAccess" },
	{ O_CopyValue, "O_CopyValue" },
	{ O_BinaryAddition, "O_BinaryAddition" },
	{ O_BinarySubtraction, "O_BinarySubtraction" },
	{ O_BinaryMultiplication, "O_BinaryMultiplication" },
	{ O_BinaryDivision, "O_BinaryDivision" },
	{ O_BinaryMod, "O_BinaryMod" },
	{ O_AddAssign, "O_AddAssign" },
	{ O_SubtractAssign, "O_SubtractAssign" },
	{ O_ModAssign, "O_ModAssign" },
	{ O_MultiplyAssign, "O_MultiplyAssign" },
	{ O_DivideAssign, "O_DivideAssign" },
	{ O_NewUnit, "O_NewUnit" },
	{ O_LoadFromContext, "O_LoadFromContext" },
	{ O_AddArrayElement, "O_AddArrayElement" },
	{ O_AddTableElement, "O_AddTableElement" },
	{ O_DereferenceFromTable, "O_DereferenceFromTable" },
	{ O_PushIterator, "O_PushIterator" },
	{ O_IteratorGetNextKeyValueOrBranch, "O_IteratorGetNextKeyValueOrBranch" },
	{ O_IteratorGetNextValueOrBranch, "O_IteratorGetNextValueOrBranch" },
	{ O_CoerceToInt, "O_CoerceToInt" },
	{ O_CoerceToString, "O_CoerceToString" },
	{ O_CoerceToWideString, "O_CoerceToWideString" },
	{ O_CoerceToFloat, "O_CoerceToFloat" },
	{ O_Switch, "O_Switch" },

	{ 0, "UNKNOWN_OPCODE" },
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
	{ "", 0, 0, 0, 0, 0, 0, 0 }, // placeholder operator for when the value can be pre-computed

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

	{ "--",   3, O_PreDecrement, true, true, false, false, false },
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
	{ "wstring", 3, O_CoerceToWideString, false, false, false, false, true },

	{ 0, 0, 0, 0, 0, 0, 0, 0 },
};
const int c_highestPrecedence = 17;

//------------------------------------------------------------------------------
struct ExpressionContext
{
	ExpressionContext( Cstr& c, CLinkList<LinkEntry>& s ) : code(c), symbols(s) { oper.add(); }

	Cstr& code;
	CLinkList<LinkEntry>& symbols;
	CLinkList< CLinkList<const Operation*> > oper;
};

bool isReserved( Cstr& token );

const Cstr& formatSymbol( Callisto_Context* C, unsigned int key );
const Cstr& formatValue( Value const& Value );
const Cstr& formatType( int type );
const Cstr& formatOpcode( int opcode );
void dumpStack( Callisto_ExecutionContext* E );
void dumpSymbols( Callisto_Context* CC );
void clearContext( Callisto_Context* CC );

void putValue( Cstr& code, Value& Value );
void putString( Cstr& code, Cstr& string );
void putWString( Cstr& code, Wstr& string );
void popOne( Callisto_ExecutionContext* E );
void popNum( Callisto_ExecutionContext* E, const int number );

void getLiteralFromCodeStream( Callisto_ExecutionContext* E, Value& Value );
void readStringFromCodeStream( Callisto_ExecutionContext* E, Cstr& string );
void readWStringFromCodeStream( Callisto_ExecutionContext* E, Wstr& string );
int doAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doCompareEQ( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doCompareGT( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doCompareLT( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doLogicalAnd( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doLogicalOr( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doMemberAccess( Callisto_ExecutionContext* E, const unsigned int key );
int doGlobalAccess( Callisto_ExecutionContext* E, const unsigned int key );

int doLogicalNot( Callisto_ExecutionContext* E, const int operand );
int doBitwiseNot( Callisto_ExecutionContext* E, const int operand );
int doPostIncrement( Callisto_ExecutionContext* E, const int operand );
int doPostDecrement( Callisto_ExecutionContext* E, const int operand );
int doPreIncrement( Callisto_ExecutionContext* E, const int operand );
int doPreDecrement( Callisto_ExecutionContext* E, const int operand );
int doNegate( Callisto_ExecutionContext* E, const int operand );
int doCopyValue( Callisto_ExecutionContext* E, const int operand );
int doCoerceToInt( Callisto_ExecutionContext* E, const int operand );
int doCoerceToFloat( Callisto_ExecutionContext* E, const int operand );
int doCoerceToString( Callisto_ExecutionContext* E, const int operand );
int doCoerceToWString( Callisto_ExecutionContext* E, const int operand );
int doSwitch( Callisto_ExecutionContext* E );

int doPushIterator( Callisto_ExecutionContext* E );
int doIteratorGetNextKeyValueOrBranch( Callisto_ExecutionContext* E );
int doIteratorGetNextValueOrBranch( Callisto_ExecutionContext* E );

int doBinaryAddition( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinarySubtraction( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinaryMultiplication( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinaryDivision( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doAddAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doSubtractAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doModAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doDivideAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doMultiplyAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinaryMod( Callisto_ExecutionContext* E, const int operand1, const int operand2 );

int doBinaryBitwiseRightShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinaryBitwiseLeftShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinaryBitwiseOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinaryBitwiseAND( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinaryBitwiseNOT( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinaryBitwiseXOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doXORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doANDAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doLeftShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doRightShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );

int doBinaryAddAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
int doBinarySubtractAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );

struct CodeHeader
{
	int32_t version;
	int32_t unitCount;
	uint32_t firstUnitBlock;
	int32_t symbols;
	uint32_t CRC;
};

int execute( Callisto_ExecutionContext* E, const unsigned int offset, const int stackFloor );

void ensureStack( Callisto_ExecutionContext* E, int to );

inline Value* getNextStackEntry( Callisto_ExecutionContext *E )
{
	if ( ++E->stackPointer >= E->stackSize )
	{
		ensureStack( E, E->stackPointer );
	}

	return E->stack + (E->stackPointer - 1);
}

bool getToken( Compilation& comp, Cstr& token, Value& value );

void valueMove( Value* target, Value* source );
void valueSwap( Value* v1, Value* v2 );
void valueDeepCopy( Value* target, Value* source );

//------------------------------------------------------------------------------
inline void valueMirror( Value* target, Value* source )
{
	clearValue( *target );
	
	target->ref1 = source->ref1;

	if ( (target->type = source->type) & BIT_PASS_BY_REF )
	{
		switch( (source->type) )
		{
			case CTYPE_STRING: target->refCstr = source->refCstr->getReference(); break;
			case CTYPE_WSTRING: target->refWstr = source->refWstr->getReference(); break;
			case CTYPE_UNIT: target->refLocalSpace = source->refLocalSpace->getReference(); break;
			case CTYPE_TABLE: target->refLocalSpace = source->refLocalSpace->getReference(); break;
			case CTYPE_ARRAY: target->refArray = source->refArray->getReference(); break;
		}
	}
}


void addArrayElement( Callisto_ExecutionContext* E, unsigned int index );
int addTableElement( Callisto_ExecutionContext* E );
void dereferenceFromTable( Callisto_ExecutionContext* E );

int Callisto_tests( int runTest =0 );

#endif
