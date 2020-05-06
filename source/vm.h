#ifndef VM_H
#define VM_H
/*------------------------------------------------------------------------------*/

#include "../include/callisto.h"

#include <stdlib.h>
#include <stdint.h>

#include "value.h"
#include "object_tpool.h"
#include "linkhash.h"
#include "linklist.h"
#include "hashtable.h"
#include "hash.h"
#include "str.h"
#include "log.h"
#include "locks.h"
#include "event.h"
#include "opcode.h"

#define D_OPERATOR(a) //a
#define D_ARG_SETUP(a) //a

extern CLog Log;

extern int g_Callisto_lastErr;
extern int g_Callisto_warn;

struct ExecutionFrame;

//------------------------------------------------------------------------------
// a thread of execution that has its own unit space of instantiated
// units and values keyed by label
struct Callisto_ExecutionContext
{
	int numberOfArgs;
	Value* Args;
	Value object; // if something calls this is the thing that called
	void* userData;

	int stackPointer;
	int stackSize;
	Value* stack;

	ExecutionFrame* frame;
	unsigned int textOffsetToResume;
	
	long threadId;
	Callisto_ThreadState state;

	static CObjectTPool<ExecutionFrame> m_framePool;

	Callisto_Context* callisto;

	inline void clear();

	Callisto_ExecutionContext()
	{
		frame = m_framePool.get();
		stack = 0;
		stackSize = 0;
		stackPointer = 0;
	}
	
	~Callisto_ExecutionContext()
	{
		clear();
		m_framePool.release( frame );
	}
};

//------------------------------------------------------------------------------
struct ExecutionFrame
{
	int returnVector;
	int onParent;
	int numberOfArguments;
	Value unitContainer; // autoritative location of Definition and namespace
	ExecutionFrame* next;

	static void clear( ExecutionFrame& F ) 
	{
		clearValue( F.unitContainer );
	}

	ExecutionFrame() { unitContainer.type32 = CTYPE_NULL; unitContainer.i64 = 0; }
};

//------------------------------------------------------------------------------
void Callisto_ExecutionContext::clear()
{
	clearValue( object );
	delete[] stack;
	stack = 0;
	stackSize = 0;
	ExecutionFrame::clear( *frame );
}

//------------------------------------------------------------------------------
struct TextSection
{
	unsigned int sourceOffset;
	unsigned int textOffsetTop;
};

// Callisto_Context holds global info, it has a "root unit" that has no parents and represents global namespace
// - pools all values and units keyvalues pool themselves

// Callisto_ExecutionContext is a thread executing inside Callisto_Context
// a Frame is an execution frame for calling a unit, it points to the current unit instantation
// a unitDefinition is a unit definition
// A Unit represents an instantiation of a unitDefinition, it has its own namespace

//------------------------------------------------------------------------------
struct Callisto_Context
{
	Callisto_RunOptions options;
	
	Cstr text; // code hangs here
	CLinkList<TextSection> textSections;
	CHashTable<Cstr> symbols; // to reverse hashes (if loaded)
	
	Cstr err; // if an error is returned, it is loaded here

	Value rootUnit;
	CHashTable<Value>* globalNameSpace; // cached from the above
		
	CHashTable<UnitDefinition> unitDefinitions;
	CHashTable<CHashTable<UnitDefinition>> typeFunctions;

	CLinkHash<Callisto_ExecutionContext> threads; // independant threads operating in this space
	CLinkHash<Callisto_ExecutionContext>::Iterator scheduler;
	
	long threadIdGenerator;
	
	Callisto_Context( const Callisto_RunOptions* opt =0 )
	{
		rootUnit.allocUnit();
		globalNameSpace = rootUnit.unitSpace;
		rootUnit.u = unitDefinitions.add( 0 );
		rootUnit.u->numberOfParents = 0;
		unitDefinitions.add( Callisto_THIS_HANDLE ); // so it cannot be created
		threadIdGenerator = 0;
		scheduler.iterate( threads );

		if ( opt )
		{
			memcpy( &options, opt, sizeof(Callisto_RunOptions) );
		}
	}

	~Callisto_Context()
	{
		clearValue( rootUnit );
	}
};

int execute( Callisto_ExecutionContext* E );

const Cstr& formatSymbol( Callisto_Context* C, unsigned int key );
const Cstr& formatValue( Value const& Value );
const Cstr& formatType( int type );
const Cstr& formatOpcode( int opcode );
void dumpStack( Callisto_ExecutionContext* E );
void dumpSymbols( Callisto_Context* C );
Callisto_ExecutionContext* getExecutionContext( Callisto_Context* C, const long threadId =0 );

#define popOne(E) { Value& V = *((E)->stack + --(E)->stackPointer); clearValue(V); }
#define popNum(E,n) { for( int i=0; i<(n); ++i ) { popOne(E); } }
#define getValueFromArg(E, arg)	(((E)->numberOfArgs > (arg)) ? (((E)->Args[(arg)].type == CTYPE_REFERENCE) ? (E)->Args[(arg)].v : (E)->Args + (arg)) : 0)

void getLiteralFromCodeStream( Callisto_ExecutionContext* E, Value& Value );
void readStringFromCodeStream( Cstr& string, const char** T );
void doAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doCompareEQ( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doCompareGT( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doCompareLT( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doLogicalAnd( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doLogicalOr( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doMemberAccess( Callisto_ExecutionContext* E, const unsigned int key );

void doNegate( Callisto_ExecutionContext* E, const int operand );
void doCoerceToInt( Callisto_ExecutionContext* E, const int operand );
void doCoerceToFloat( Callisto_ExecutionContext* E, const int operand );
void doCoerceToString( Callisto_ExecutionContext* E, const int operand );
void doSwitch( Callisto_ExecutionContext* E, const char** T );

void doPushIterator( Callisto_ExecutionContext* E );

void doBinaryAddition( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinarySubtraction( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinaryMultiplication( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinaryDivision( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinaryMod( Callisto_ExecutionContext* E, const int operand1, const int operand2 );

void doAddAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doSubtractAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doModAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doDivideAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doMultiplyAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );

void doBinaryBitwiseRightShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinaryBitwiseLeftShift( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinaryBitwiseOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinaryBitwiseAND( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinaryBitwiseNOT( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doBinaryBitwiseXOR( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doXORAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doANDAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doLeftShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );
void doRightShiftAssign( Callisto_ExecutionContext* E, const int operand1, const int operand2 );

void addArrayElement( Callisto_ExecutionContext* E, unsigned int index );
void addTableElement( Callisto_ExecutionContext* E );
void dereferenceFromTable( Callisto_ExecutionContext* E );

bool isValidLabel( Cstr& token, bool doubleColonOkay =false );

int loadEx( Callisto_Context* C, const char* inData, const unsigned int inLen );

//------------------------------------------------------------------------------
inline Value* getStackValue( Callisto_ExecutionContext* E, int offset )
{
	Value* value = E->stack + (E->stackPointer - offset);
	return (value->type == CTYPE_REFERENCE) ? value->v : value;
}

//------------------------------------------------------------------------------
inline Value* getValueFromHandle( Callisto_Context* C, Callisto_Handle H, bool createIfNotFound =false )
{
	Value *V = C->globalNameSpace->get( H );
	if ( !V )
	{
		if ( createIfNotFound )
		{
			return C->globalNameSpace->add( H );
		}
		
		return 0;
	}

	return (V->type == CTYPE_REFERENCE) ? V->v : V;
}

//------------------------------------------------------------------------------
inline void ensureStack( Callisto_ExecutionContext* E )
{
	int oldStackSize = E->stackSize;
	Value* old = E->stack;

	E->stackSize = (E->stackSize * 3) / 2;

	E->stack = new Value[ E->stackSize ];

	for( int i=0; i<oldStackSize; ++i )
	{
		E->stack[i].ref1 = old[i].ref1;
		E->stack[i].ref2 = old[i].ref2;
		E->stack[i].type32 = old[i].type32;
		old[i].ref1 = 0;
		old[i].ref2 = 0;
		old[i].type = CTYPE_NULL;
	}

	delete[] old;
}

#define BINARY_ARG_SETUP \
Value* from = E->stack + (E->stackPointer - operand1); \
Value* to = E->stack + (E->stackPointer - operand2); \
Value* destination = to; \
from = (from->type == CTYPE_REFERENCE) ? from->v : from; \
to = (to->type == CTYPE_REFERENCE) ? to->v : to; \
to->flags &= ~VFLAG_HASH_COMPUTED; \
D_ARG_SETUP(Log("Binary arg setup from operand1[%d] to operand2[%d]", E->stackPointer - operand2, E->stackPointer - operand1)); \

#define UNARY_ARG_SETUP \
Value* value = E->stack + (E->stackPointer - operand); \
Value* destination = value; \
value = (value->type == CTYPE_REFERENCE) ? value->v : value; \
value->flags &= ~VFLAG_HASH_COMPUTED; \
D_ARG_SETUP(Log("Unary arg setup from operand[%d]", E->stackPointer - operand)); \

struct CodeHeader
{
	int32_t version;
	int32_t unitCount;
	uint32_t firstUnitBlock;
	int32_t symbols;
	uint32_t CRC;
};

//------------------------------------------------------------------------------
inline Value* getNextStackEntry( Callisto_ExecutionContext *E )
{
	if ( E->stackPointer >= E->stackSize )
	{
		ensureStack( E );
	}

	return E->stack + E->stackPointer++;
}

//------------------------------------------------------------------------------
inline void doLogicalNot( Callisto_ExecutionContext* E, const int operand )
{
	UNARY_ARG_SETUP;

	int result = value->i64 ? 0 : 1;
	clearValue( *destination );
	destination->type32 = CTYPE_INT;
	if ( value->type == CTYPE_INT )
	{
		destination->i64 = result;
	}
	else if ( value->type == CTYPE_NULL )
	{
		destination->i64 = 1;
	}
	else
	{
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
inline void doBitwiseNot( Callisto_ExecutionContext* E, const int operand )
{
	UNARY_ARG_SETUP;
	int64_t result = ~value->i64;
	clearValue( *destination );

	if ( value->type == CTYPE_INT )
	{
		destination->type32 = CTYPE_INT;
		destination->i64 = result;
	}
}

//------------------------------------------------------------------------------
inline void doPostIncrement( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type != CTYPE_REFERENCE )
	{
		clearValue( *destination );
		g_Callisto_warn = CE_LiteralPostOperatorNotPossible;
	}
	else
	{
		value = value->v;
		D_ARG_SETUP(Log("Unary arg setup from operand[%d]", E->stackPointer - operand));

		if ( value->type == CTYPE_INT )
		{
			int64_t result = value->i64++;
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
			D_OPERATOR(Log("doPostDecrement<1> [%lld]", destination->i64));
		}
		else if ( value->type == CTYPE_FLOAT )
		{
			double result = value->d++;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doPostDecrement<2> [%g]", destination->d));
		}
		else
		{
			g_Callisto_warn = CE_IncompatibleDataTypes;
			clearValue( *destination );
		}
	}
}

//------------------------------------------------------------------------------
inline void doPostDecrement( Callisto_ExecutionContext* E, const int operand )
{
	Value* value = E->stack + (E->stackPointer - operand);
	Value* destination = value;

	if ( value->type != CTYPE_REFERENCE )
	{
		clearValue( *destination );
		g_Callisto_warn = CE_LiteralPostOperatorNotPossible;
	}
	else
	{
		value = value->v;
		D_ARG_SETUP(Log("Unary arg setup from operand[%d]", E->stackPointer - operand));

		if ( value->type == CTYPE_INT )
		{
			int64_t result = value->i64--;
			clearValue( *destination );
			destination->type32 = CTYPE_INT;
			destination->i64 = result;
			D_OPERATOR(Log("doPostDecrement<1> [%lld]", destination->i64));
		}
		else if ( value->type == CTYPE_FLOAT )
		{
			double result = value->d--;
			clearValue( *destination );
			destination->type32 = CTYPE_FLOAT;
			destination->d = result;
			D_OPERATOR(Log("doPostDecrement<2> [%g]", destination->d));
		}
		else
		{
			g_Callisto_warn = CE_IncompatibleDataTypes;
			clearValue( *destination );
		}
	}
}

//------------------------------------------------------------------------------
inline void doPreIncrement( Callisto_ExecutionContext* E, const int operand )
{
	UNARY_ARG_SETUP;
	destination = destination;

	if ( value->type == CTYPE_INT )
	{
		++value->i64;
		D_OPERATOR(Log("doPreIncrement<1> [%lld]", value->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		++value->d;
		D_OPERATOR(Log("doPreIncrement<2> [%g]", value->d));
	}
	else
	{
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

//------------------------------------------------------------------------------
inline void doPreDecrement( Callisto_ExecutionContext* E, const int operand )
{
	UNARY_ARG_SETUP;
	destination = destination;

	if ( value->type == CTYPE_INT )
	{
		--value->i64;
		D_OPERATOR(Log("doPreDecrement<1> [%lld]", value->i64));
	}
	else if ( value->type == CTYPE_FLOAT )
	{
		--value->d;
		D_OPERATOR(Log("doPreDecrement<2> [%g]", value->d));
	}
	else
	{
		g_Callisto_warn = CE_IncompatibleDataTypes;
	}
}

#endif
