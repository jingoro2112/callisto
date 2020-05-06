#ifndef CALLISTO_H
#define CALLISTO_H
/*------------------------------------------------------------------------------*/

#define CALLISTO_VERSION 0x0020

#include <stdint.h>

struct Callisto_Context;
struct Callisto_ExecutionContext;

typedef int Callisto_Handle;
typedef Callisto_Handle (*Callisto_CallbackFunction)( Callisto_ExecutionContext* E );

#define Callisto_NULL_HANDLE 0
#define Callisto_THIS_HANDLE -1

struct Callisto_RunOptions
{
	bool returnOnIdle; // if no tasks can run, should it return, or spin in a nice loop waiting for work [standalone]
	bool returnOnError; // if false, interpreter will try to continue if a thread dies messily
	void (*traceCallback)( const char* code, const unsigned int line, const unsigned int col, const int thread );
	void (*warningCallback)( const int warning, const char* code, const unsigned int line, const unsigned int col, const int thread );
	bool warningsFatal;

	Callisto_RunOptions()
	{
		warningsFatal = false;
		returnOnIdle = true;
		returnOnError = true;
		traceCallback = 0;
		warningCallback = 0;
	}
};

Callisto_Context* Callisto_createContext( const Callisto_RunOptions* options =0 );
void Callisto_destroyContext( Callisto_Context* C );

enum Callisto_Error
{
	CE_NoError = 0,
	CE_FileErr = -999,
	CE_ImportMustBeString,
	CE_ImportLoadError,
	
	CE_DataErr,
	CE_IncompatibleDataTypes,
	CE_HandleNotFound,
	CE_BadVersion,
	CE_BadLabel,
	CE_NullPointer,
	CE_ArgOutOfRange,

	CE_ThreadNotFound,
	CE_UnitNotFound,
	CE_ParentNotFound,
	CE_UnitNotNative,

	CE_LiteralPostOperatorNotPossible,
	CE_TryingToAccessMembersOfNonUnit,
	CE_AssignedValueToStack,
	
	CE_UnhashableDataType,
	CE_MemberNotFound,
	CE_TypeCannotBeIterated,
	CE_IteratorNotFound,
	CE_UnrecognizedByteCode,
	CE_CannotOperateOnLiteral,
	
	CE_VMRunningOnLoad,
	CE_ThreadNotIdle,
	CE_ThreadNotWaiting,
	CE_ThreadAlreadyWaiting,
	
	CE_FirstThreadArgumentMustBeUnit,
	CE_FunctionTableNotFound,
	CE_FunctionTableEntryNotFound,
	CE_StackNotEmptyAtExecutionComplete,
	CE_CannotMixFunctionsWithCallable,

	CE_ValueIsConst,
	CE_TriedToCallNonUnit,
};

const int Callisto_lastErr();
const char* Callisto_formatError( const int err );

enum Callisto_ThreadState
{
	Runnable, // will run the next time the scheduler gets to it
	Running, // is running right now
	Waiting, // waiting for an event
	Reap, // completed, jsut waiting to be cleaned up by the scheduler
};

struct Callisto_FunctionEntry
{
	const char* functionName; // what this function is called
	Callisto_CallbackFunction function; // C-signature function
	void* userData; //  opaque pointer that will be passed to function when called
};
void Callisto_importFunction( Callisto_ExecutionContext* E, const char* functionName, Callisto_CallbackFunction function, void* userData );
void Callisto_importFunction( Callisto_Context* C, const char* functionName, Callisto_CallbackFunction function, void* userData );
void Callisto_importFunctionList( Callisto_ExecutionContext* E, const Callisto_FunctionEntry* entries, const int numberOfEntries );
void Callisto_importFunctionList( Callisto_Context* C, const Callisto_FunctionEntry* entries, const int numberOfEntries );

int Callisto_importConstant( Callisto_ExecutionContext* E, const char* name, const Callisto_Handle handle );
int Callisto_importConstant( Callisto_Context* C, const char* name, const Callisto_Handle handle );

void Callisto_importAll( Callisto_Context *C );
void Callisto_importStdlib( Callisto_Context *C );
void Callisto_importString( Callisto_Context *C );
void Callisto_importMath( Callisto_Context *C );
void Callisto_importFile( Callisto_Context *C );
void Callisto_importJson( Callisto_Context *C );
void Callisto_importIterators( Callisto_Context *C );

int Callisto_parse( const char* data, const int size, char** out, unsigned int* outLen, const bool addDebugInfo =false );

int Callisto_runFile( Callisto_Context* C, const char* fileName, const Callisto_Handle argumentValueHandle =0 );
int Callisto_run( Callisto_Context* C, const char* inData, const unsigned int inLen =0, const Callisto_Handle argumentValueHandle =0 );

int Callisto_runFileCompiled( Callisto_Context* C, const char* fileName, const Callisto_Handle argumentValueHandle =0 );
int Callisto_runCompiled( Callisto_Context* C, const char* inData, const unsigned int inLen, const Callisto_Handle argumentValueHandle =0 );

const Callisto_Handle Callisto_call( Callisto_ExecutionContext *E, const char* unitName, const Callisto_Handle argumentHandle );
const Callisto_Handle Callisto_call( Callisto_ExecutionContext *E, const char* unitName, const Callisto_Handle* argumentValueHandleList =0 );
const Callisto_Handle Callisto_call( Callisto_Context *C, const char* unitName, const Callisto_Handle argumentHandle );
const Callisto_Handle Callisto_call( Callisto_Context* C, const char* unitName, const Callisto_Handle* argumentValueHandleList =0 );

const unsigned int Callisto_getNumberOfArgs( Callisto_ExecutionContext* E );
const char* Callisto_getStringArg( Callisto_ExecutionContext* E, const int argNum, const char** data =0, unsigned int* len =0 );
const wchar_t* Callisto_getWStringArg( Callisto_ExecutionContext* E, const int argNum, wchar_t* data, unsigned int* len );
const char* Callisto_formatArg( Callisto_ExecutionContext* E, const int argNum, char* data, unsigned int* len );
const int64_t Callisto_getIntArg( Callisto_ExecutionContext* E, const int argNum );
const double Callisto_getFloatArg( Callisto_ExecutionContext* E, const int argNum );

void* Callisto_getUserData( Callisto_ExecutionContext* E );

enum Callisto_ArgType
{
	Callisto_ArgNull =0,
	Callisto_ArgInt,
	Callisto_ArgFloat,
	Callisto_ArgString,
	Callisto_ArgArray,
	Callisto_ArgTable,
	Callisto_ArgUnit,
	Callisto_ArgStringIterator,
	Callisto_ArgTableIterator,
	Callisto_ArgArrayIterator,
};
const Callisto_ArgType Callisto_getArgType( Callisto_ExecutionContext* E, const int argNum );
void Callisto_setTypeMethods( Callisto_Context* C, const Callisto_ArgType type, const Callisto_FunctionEntry* entries =0, const int numberOfEntries =0 );

const Callisto_ArgType Callisto_getHandleType( Callisto_ExecutionContext* E, const Callisto_Handle handle );
const Callisto_ArgType Callisto_getHandleType( Callisto_Context* C, const Callisto_Handle handle );
const int Callisto_handleToString( Callisto_Context* C, const Callisto_Handle handle, const char** buf, int* len );
const int Callisto_handleToString( Callisto_ExecutionContext* E, const Callisto_Handle handle, const char** buf, int* len );
const int Callisto_handleToFloat( Callisto_Context* C, const Callisto_Handle handle, double* value );
const int Callisto_handleToFloat( Callisto_ExecutionContext* E, const Callisto_Handle handle, double* value );
const int Callisto_handleToInt( Callisto_Context* C, const Callisto_Handle handle, int64_t* value );
const int Callisto_handleToInt( Callisto_ExecutionContext* E, const Callisto_Handle handle, int64_t* value );

const Callisto_Handle Callisto_setValue( Callisto_ExecutionContext* E, const int64_t i, const Callisto_Handle handle =0 );
const Callisto_Handle Callisto_setValue( Callisto_ExecutionContext* E, const double d, const Callisto_Handle handle =0 );
const Callisto_Handle Callisto_setValue( Callisto_ExecutionContext* E, const char* string, const int len =-1, const Callisto_Handle handle =0 );
const Callisto_Handle Callisto_setValue( Callisto_ExecutionContext* E, const wchar_t* wstring, const int len =-1, const Callisto_Handle handle =0 );

const Callisto_Handle Callisto_setValue( Callisto_Context* C, const int64_t i, const Callisto_Handle handle =0 );
const Callisto_Handle Callisto_setValue( Callisto_Context* C, const double d, const Callisto_Handle handle =0 );
const Callisto_Handle Callisto_setValue( Callisto_Context* C, const char* string, const int len =0, const Callisto_Handle handle =0 );
const Callisto_Handle Callisto_setValue( Callisto_Context* C, const wchar_t* wstring, const int len =0, const Callisto_Handle handle =0 );

const Callisto_Handle Callisto_createArray( Callisto_ExecutionContext* E, const Callisto_Handle handle =0 );
const int Callisto_setArrayValue( Callisto_ExecutionContext* E, const Callisto_Handle array, const Callisto_Handle value, const int index =-1 );
const int Callisto_getArrayCount( Callisto_ExecutionContext* E, const Callisto_Handle array );
const Callisto_Handle Callisto_getArrayValue( Callisto_ExecutionContext* E, const Callisto_Handle array, const int index );

const Callisto_Handle Callisto_createTable( Callisto_ExecutionContext* E, const Callisto_Handle handle =0 );
const int Callisto_getTableCount( Callisto_ExecutionContext* E, const Callisto_Handle table );
const int Callisto_setTableValue( Callisto_ExecutionContext* E, const Callisto_Handle table, const Callisto_Handle key, const Callisto_Handle value );
const Callisto_Handle Callisto_getTableValue( Callisto_ExecutionContext* E, const Callisto_Handle table, const Callisto_Handle key );

const Callisto_Handle Callisto_createArray( Callisto_Context* C, const Callisto_Handle handle =0 );
const int Callisto_getArrayCount( Callisto_Context* C, const Callisto_Handle array );
const int Callisto_setArrayValue( Callisto_Context* C, const Callisto_Handle array, const Callisto_Handle value, const int index =-1 );
const Callisto_Handle Callisto_getArrayValue( Callisto_Context* C, const Callisto_Handle array, const int index );

const Callisto_Handle Callisto_createTable( Callisto_Context* C, const Callisto_Handle handle =0 );
const int Callisto_getTableCount( Callisto_Context* C, const Callisto_Handle table );
const int Callisto_setTableValue( Callisto_Context* C, const Callisto_Handle table, const Callisto_Handle key, const Callisto_Handle value );
const Callisto_Handle Callisto_getTableValue( Callisto_Context* C, const Callisto_Handle table, const Callisto_Handle key );

void Callisto_releaseValue( Callisto_ExecutionContext* E, const Callisto_Handle handle );
void Callisto_releaseValue( Callisto_Context* C, const Callisto_Handle handle );

// not really implemented yet.. not even sure this is HOW I want to implement it
const int Callisto_resume( Callisto_ExecutionContext *E, const Callisto_Handle argumentValueHandle =0 );
const int Callisto_resume( Callisto_Context* C, const long threadId, const Callisto_Handle argumentValueHandle =0 );
const Callisto_ThreadState Callisto_getThreadState( Callisto_ExecutionContext* E );
const Callisto_ThreadState Callisto_getThreadState( Callisto_ExecutionContext* E, Callisto_Context* C, const long threadId );
const long Callisto_getThreadId( Callisto_ExecutionContext* E );
const int Callisto_yield( Callisto_ExecutionContext *E );
const int Callisto_wait( Callisto_ExecutionContext *E, const Callisto_Handle argumentValueHandle =0 );


#endif
