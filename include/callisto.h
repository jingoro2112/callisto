#ifndef CALLISTO_H
#define CALLISTO_H
/*------------------------------------------------------------------------------*/

#define CALLISTO_VERSION 0x0001

#include <stdint.h>

struct Unit;
struct Value;
struct Callisto_Context;
struct Callisto_ExecutionContext;

Callisto_Context* Callisto_createContext();
Callisto_ExecutionContext* Callisto_createExecutionContext( Callisto_Context* C );
void Callisto_destroyContext( Callisto_Context* C );
void Callisto_destroyExecutionContext( Callisto_ExecutionContext* E );

//------------------------------------------------------------------------------
enum Callisto_Error
{
	CE_NoError = 0,
	CE_FileErr = -1,
	CE_DataErr = -2,
	CE_BadVersion = -3,
	CE_ThreadNotFound = -4,
	CE_UnitNotFound = -5,
	CE_ParentNotFound = -6,
	CE_IncompatibleDataTypes = -7,
	CE_StackedPostOperatorsUnsupported = -8,
	CE_AssignedValueToStack = -9,
	CE_UnhashableDataType = -10,
	CE_TryingToAccessMembersOfNonUnit = -11,
	CE_MemberNotFound = -12,
	CE_TypeCannotBeIterated = -13,
	CE_UnrecognizedByteCode = -14,
	CE_CannotOperateOnLiteral = -15,
	CE_HandleNotFound = -16,
};

//------------------------------------------------------------------------------
enum Callisto_TypeBits
{
	BIT_TYPE_0 = 1<<0,
	BIT_TYPE_1 = 1<<1,
	BIT_TYPE_2 = 1<<2,
	BIT_TYPE_3 = 1<<3,
	
	BIT_PASS_BY_REF = 1<<5, // if set, this value is passed by reference only
	BIT_CAN_HASH = 1<<6,
};

//------------------------------------------------------------------------------
enum Callisto_DataType
{
	CTYPE_NULL     = 0 | BIT_CAN_HASH,
	CTYPE_INT      = 1 | BIT_CAN_HASH,
	CTYPE_FLOAT    = 2 | BIT_CAN_HASH,
	CTYPE_STRING   = 3 | BIT_PASS_BY_REF | BIT_CAN_HASH,
	CTYPE_WSTRING  = 4 | BIT_PASS_BY_REF | BIT_CAN_HASH,
	CTYPE_UNIT     = 5 | BIT_PASS_BY_REF,
	CTYPE_TABLE    = 6 | BIT_PASS_BY_REF,
	CTYPE_ARRAY    = 7 | BIT_PASS_BY_REF,

	CTYPE_OFFSTACK_REFERENCE = 8, // this value is a placeholder on the stack for the actual value held in *v

	CTYPE_ARRAY_ITERATOR = 9 | BIT_PASS_BY_REF,
	CTYPE_HASH_ITERATOR = 10 | BIT_PASS_BY_REF,
	CTYPE_STRING_ITERATOR = 11 | BIT_PASS_BY_REF,
	CTYPE_WSTRING_ITERATOR = 12 | BIT_PASS_BY_REF,
	// 10 reserved
	// 11 reserved
	// 12 reserved
	// 13 reserved
	// 14 reserved
	// 15 reserved
};

typedef unsigned int Callisto_Handle;
typedef Callisto_Handle (*Callisto_CallbackFunction)( Callisto_ExecutionContext* context );

#define Callisto_EMPTY_HANDLE 0

//------------------------------------------------------------------------------
struct Callisto_FunctionEntry
{
	const char* label;
	Callisto_CallbackFunction function;
	void* userData;
};
void Callisto_importList( Callisto_Context* C, const Callisto_FunctionEntry* entries, const int numberOfEntries );
void Callisto_importFunction( Callisto_Context* C, const char* label, Callisto_CallbackFunction function, void* userData =0 );

void Callisto_importStdlib( Callisto_Context *C );
void Callisto_importString( Callisto_Context *C );

void Callisto_clearGlobalMetaTable( Callisto_Context* C, const int type );
void Callisto_setGlobalMetaTable( Callisto_Context* C, const int type, const Callisto_FunctionEntry* entries, const int numberOfEntries );

const int Callisto_lastErr();
const char* Callisto_formatError( int err );

int Callisto_parse( const char* data, const int size, char** out, unsigned int* outLen );

// load a file (if it is pre-parsed, great, otherwise try and run 'parse' on it) and run to establish globals
int Callisto_loadFile( Callisto_ExecutionContext *E, const char* fileName, const Callisto_Handle argumentValueHandle =0 );
int Callisto_load( Callisto_ExecutionContext *E, const char* inData, const unsigned int inLen =0, const Callisto_Handle argumentValueHandle =0 );

int Callisto_execute( Callisto_ExecutionContext *E, const char* unitName, const Callisto_Handle argumentValueHandle =0 );

const char* Callisto_getStringArg( Callisto_ExecutionContext* E, const int argNum, char* data =0, unsigned int* len =0 );
const wchar_t* Callisto_getWStringArg( Callisto_ExecutionContext* E, const int argNum, wchar_t* data =0, unsigned int* len =0 );
int64_t Callisto_getIntArg( Callisto_ExecutionContext* E, const int argNum );
double Callisto_getFloatArg( Callisto_ExecutionContext* E, const int argNum );
void* Callisto_getUserData( Callisto_ExecutionContext* E );
const unsigned int Callisto_getNumberOfArgs( Callisto_ExecutionContext* E );

int Callisto_getArgType( Callisto_ExecutionContext* E, const int argNum );

const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E );
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E, const int64_t i );
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E, const double d );
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E, const char* string, const int len =0 );
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E, const wchar_t* string, const int len =0 );

int Callisto_setValue( Callisto_ExecutionContext* E, const Callisto_Handle loadToHandle, const int64_t i );
int Callisto_setValue( Callisto_ExecutionContext* E, const Callisto_Handle loadToHandle, const double d );
int Callisto_setValue( Callisto_ExecutionContext* E, const Callisto_Handle loadToHandle, const char* string, const int len =0 );
int Callisto_setValue( Callisto_ExecutionContext* E, const Callisto_Handle loadToHandle, const wchar_t* string, const int len =0 );

int Callisto_setArrayValue( Callisto_ExecutionContext* E, const Callisto_Handle handleOfArray, const int index, const Callisto_Handle handleOfData );
int Callisto_addArrayValue( Callisto_ExecutionContext* E, const Callisto_Handle handleOfArray, const Callisto_Handle handleOfData );
int Callisto_getArrayCount( Callisto_ExecutionContext* E, const Callisto_Handle handleOfArray );
int Callisto_getArrayValue( Callisto_ExecutionContext* E, const int index );

int Callisto_setTableValue( Callisto_ExecutionContext* E, const Callisto_Handle handleOfTable, const Callisto_Handle handleOfKey, const Callisto_Handle handleOfData );
int Callisto_getTableCount( Callisto_ExecutionContext* E, const Callisto_Handle handleOfTable );
int Callisto_getTableValue( Callisto_ExecutionContext* E, const Callisto_Handle handleOfArray, const Callisto_Handle handleOfKey );

void Callisto_releaseValue( Callisto_ExecutionContext* E, const Callisto_Handle handle );


#endif
