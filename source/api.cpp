#include "arch.h"
#include "c_log.h"
#include "vm.h"
#include "utf8.h"
#include "array.h"

Callisto::CLog C_Log;

#define D_LOAD(a) //a
#define D_EXECUTE(a) //a
#define D_IMPORT(a) //a

//------------------------------------------------------------------------------
Callisto_Context* Callisto_createContext( const Callisto_RunOptions* options )
{
	return new Callisto_Context( options );
}

//------------------------------------------------------------------------------
void Callisto_destroyContext( Callisto_Context* C )
{
	delete C;
}

//------------------------------------------------------------------------------
const int Callisto_lastErr()
{
	return g_Callisto_lastErr;
}

//------------------------------------------------------------------------------
void Callisto_importFunctionList( Callisto_ExecutionContext* E, const Callisto_FunctionEntry* entries, const int numberOfEntries )
{
	Callisto_importFunctionList( E->callisto, entries, numberOfEntries );
}

//------------------------------------------------------------------------------
void Callisto_importFunctionList( Callisto_Context* C, const Callisto_FunctionEntry* entries, const int numberOfEntries )
{
	for( int i=0; i<numberOfEntries; i++ )
	{
		unsigned int key = hash32(entries[i].functionName, strlen(entries[i].functionName) );

		UnitDefinition* unitDefinition = C->unitDefinitions.get( key );
		if ( unitDefinition )
		{
			unitDefinition->childUnits.clear();
		}
		else
		{
			unitDefinition = C->unitDefinitions.add( key );
		}

		unitDefinition->function = entries[i].function;
		unitDefinition->userData = entries[i].userData;
		unitDefinition->textOffset = 0;

		D_IMPORT(C_Log("imported function [%s] [0x%08X]", entries[i].functionName, key));
	}
}

//------------------------------------------------------------------------------
void Callisto_importAll( Callisto_Context *C )
{
	Callisto_importStdlib( C );
	Callisto_importString( C );
	Callisto_importMath( C );
	Callisto_importFile( C );
	Callisto_importJson( C );
	Callisto_importIterators( C );
	Callisto_importSystem( C );
}

//------------------------------------------------------------------------------
void Callisto_importFunction( Callisto_ExecutionContext* E, const char* functionName, Callisto_CallbackFunction function, void* userData )
{
	Callisto_importFunction( E->callisto, functionName, function, userData );
}

//------------------------------------------------------------------------------
void Callisto_importFunction( Callisto_Context* C, const char* functionName, Callisto_CallbackFunction function, void* userData )
{
	Callisto_FunctionEntry entry;
	entry.functionName = functionName;
	entry.function = function;
	entry.userData = userData;
	Callisto_importFunctionList( C, &entry, 1 );
}

//------------------------------------------------------------------------------
const char* Callisto_formatError( const int err )
{
	static C_str ret;
	switch( err )
	{
		default: return ret = "UNKNOWN ERROR";

		case CE_NoError: return ret = "CE_NoError";
		case CE_FileErr: return ret = "CE_FileErr";
		case CE_ImportMustBeString: return ret = "CE_ImportMustBeString";

		case CE_DataErr: return ret = "CE_DataErr";
		case CE_IncompatibleDataTypes: return ret = "CE_IncompatibleDataTypes";
		case CE_HandleNotFound: return ret = "CE_HandleNotFound";
		case CE_BadVersion: return ret = "CE_BadVersion";
		case CE_BadLabel: return ret = "CE_BadLabel";
		case CE_NullPointer: return ret = "CE_NullPointer";
		case CE_ArgOutOfRange: return ret = "CE_ArgOutOfRange";

		case CE_ThreadNotFound: return ret = "CE_ThreadNotFound";
		case CE_UnitNotFound: return ret = "CE_UnitNotFound";
		case CE_ParentNotFound: return ret = "CE_ParentNotFound";
		case CE_UnitNotNative: return ret = "CE_UnitNotNative";

		case CE_LiteralPostOperatorNotPossible: return ret = "CE_LiteralPostOperatorNotPossible";
		case CE_TryingToAccessMembersOfNonUnit: return ret = "CE_TryingToAccessMembersOfNonUnit";
		case CE_AssignedValueToStack: return ret = "CE_AssignedValueToStack";

		case CE_UnhashableDataType: return ret = "CE_UnhashableDataType";
		case CE_MemberNotFound: return ret = "CE_MemberNotFound";
		case CE_TypeCannotBeIterated: return ret = "CE_TypeCannotBeIterated";
		case CE_UnrecognizedByteCode: return ret = "CE_UnrecognizedByteCode";
		case CE_CannotOperateOnLiteral: return ret = "CE_CannotOperateOnLiteral";

		case CE_VMRunningOnLoad: return ret = "CE_VMRunningOnLoad";
		case CE_ThreadNotIdle: return ret = "CE_ThreadNotIdle";
		case CE_ThreadNotWaiting: return ret = "CE_ThreadNotWaiting";
		case CE_ThreadAlreadyWaiting: return ret = "CE_ThreadAlreadyWaiting";

		case CE_FirstThreadArgumentMustBeUnit: return ret = "CE_FirstThreadArgumentMustBeUnit";
		case CE_FunctionTableNotFound: return ret = "CE_FunctionTableNotFound";
		case CE_FunctionTableEntryNotFound: return ret = "CE_FunctionTableEntryNotFound";
		case CE_StackNotEmptyAtExecutionComplete: return ret = "CE_StackNotEmptyAtExecutionComplete";
		case CE_CannotMixFunctionsWithCallable: return ret = "CE_CannotMixFunctionsWithCallable";

		case CE_ValueIsConst: return ret = "CE_ValueIsConst";
		case CE_TriedToCallNonUnit: return ret = "CE_TriedToCallNonUnit";
	}
}

//------------------------------------------------------------------------------
static int runScheduler( Callisto_Context *C )
{
	int64_t waitFor;
	bool workDone;

	for(;;)
	{
		Callisto_ExecutionContext *E = C->scheduler.getFirst();
		waitFor = 0;
		workDone = false;
		while( E )
		{
			if ( E->state & Callisto_ThreadState::Runnable )
			{
				if ( E->sleepUntil )
				{
					if ( E->sleepUntil > Callisto_getCurrentMilliseconds() )
					{
						if ( E->sleepUntil > waitFor )
						{
							waitFor = E->sleepUntil;
						}

						E = C->scheduler.getNext();
						continue;
					}
					else
					{
						E->sleepUntil = 0;
					}
				}
				
				E->state |= Callisto_ThreadState::Running;
				
				if ( execute(E) )
				{
					E->state |= Callisto_ThreadState::Reap;
					
					if ( C->options.returnOnError )
					{
						return g_Callisto_lastErr;
					}
				}

				workDone = true;

				E->state &= ~Callisto_ThreadState::Running;
			}

			if ( E->state & Callisto_ThreadState::Reap )
			{
				if ( E->stackPointer != 1 )
				{
					popNum( E, (E->stackPointer - 1) );
					E->warning = CE_StackNotEmptyAtExecutionComplete;
				}

				C->scheduler.removeCurrent();
				
				E = C->scheduler.getCurrent();
			}
			else
			{
				E = C->scheduler.getNext();
			}
		}

		if ( waitFor )
		{
			int64_t current = Callisto_getCurrentMilliseconds();
			if ( waitFor > current )
			{
				C->OSWaitHandle.wait( (unsigned int)(waitFor - current) );
			}
			
			waitFor = 0;
		}
		else if ( !workDone )
		{
			C->OSWaitHandle.wait( 500 );
		}
		
		if ( !C->scheduler.count() && C->options.returnOnIdle )
		{
			return CE_NoError;
		}
	}
}

//------------------------------------------------------------------------------
inline Callisto_Handle getUniqueHandle( Callisto_Context* C, Value** val )
{
	static Callisto_Handle s_handle = 0;
	
	while( C->globalNameSpace->get(++s_handle) );
	
	*val = C->globalNameSpace->add( s_handle);
	
	return s_handle;
}

//------------------------------------------------------------------------------
inline Callisto_Handle setValueSetup( Callisto_Context* C, const Callisto_Handle handle, Value** val )
{
	if ( !handle )
	{
		return getUniqueHandle( C, val ); // val will be set to a new null
	}

	*val = C->globalNameSpace->get( handle );
	if ( !*val ) // wait what? we were passed a handle that didn't exist? abort!
	{
		return 0;
	}

	clearValue( **val ); // clear out whatever it was

	return handle;
}

//------------------------------------------------------------------------------
int Callisto_runFileCompiled( Callisto_Context* C, const char* fileName, const Callisto_Handle argumentValueHandle )
{
	C_str file;
	if ( !file.fileToBuffer(fileName) )
	{
		return g_Callisto_lastErr = CE_FileErr;
	}
	
	return Callisto_runCompiled( C, file.c_str(), file.size(), argumentValueHandle );
}

//------------------------------------------------------------------------------
int loadEx( Callisto_Context* C, const char* inData, const unsigned int inLen )
{
	const CodeHeader *header = (CodeHeader *)inData;
	if ( (inLen < sizeof(CodeHeader)) || (ntohl(header->CRC) != hash32(header, sizeof(CodeHeader) - 4)) )
	{
		return g_Callisto_lastErr = CE_DataErr;
	}

	header = (CodeHeader *)inData;
	if ( ntohl(header->version) != CALLISTO_VERSION )
	{
		D_LOAD(C_Log("bad version [0x%08X]", ntohl(header->version)));
		return g_Callisto_lastErr = CE_BadVersion;
	}

	TextSection* section = C->textSections.addTail();

	int textLocation = (int)C->text.size();
	C->text.append( inData, inLen );

	section->textOffsetTop = C->text.size();

	D_LOAD(C_Log("first block [0x%08X]", ntohl(header->firstUnitBlock)));

	const char* T = C->text.c_str() + ntohl(header->firstUnitBlock) + textLocation;

	// load name mappings
	for( int i=0; i<(int)ntohl(header->symbols); i++ )
	{
		unsigned int key = ntohl(*(unsigned int *)T);
		T += 4;
		C_str* label = C->symbols.get( key );
		if ( !label )
		{
			label = C->symbols.add( key );
		}
		char size = *T++;
		label->set( T, size );
		T += size;

		D_LOAD(C_Log("loaded symbol [%s:0x%08X]", label->c_str(), key));
	}

	for( int i=0; i<(int)ntohl(header->unitCount); i++ )
	{
		C_str name;
		readStringFromCodeStream( name, &T );

		unsigned int key = ntohl( *(unsigned int *)T );
		T += 4;

		D_LOAD(C_Log("Adding unit [%s] signature [0x%08X]", name.c_str(), key ));

		UnitDefinition* unitDefinition = C->unitDefinitions.get( key );
		if ( unitDefinition )
		{
			unitDefinition->childUnits.clear();
		}
		else
		{
			unitDefinition = C->unitDefinitions.add( key );
		}

		unitDefinition->textOffset = textLocation + ntohl(*(unsigned int *)T );
		T += 4;
		unitDefinition->function = 0;

		unitDefinition->member = *T++ != 0;
		
		unitDefinition->numberOfParents = (int)(*(char *)T);
		T++;

		D_LOAD(C_Log("unitDefinition [%s]@[%d] [%d]parents", name.c_str(), unitDefinition->textOffset, unitDefinition->numberOfParents));

		for( int j=0; j<unitDefinition->numberOfParents; j++ )
		{
			unitDefinition->parentList[j] = ntohl(*(unsigned int *)T);
			D_LOAD(C_Log("parent[0x%08X]", unitDefinition->parentList[j]));
			T += 4;
		}
		unitDefinition->parentList[unitDefinition->numberOfParents] = key;
		D_LOAD(C_Log("unitDefinition[0x%08X]", key));
		++unitDefinition->numberOfParents;

		int children = ntohl(*(unsigned int *)T);
		T += 4;

		D_LOAD(C_Log("%d children", children));
		for( int j=0; j<children; j++ )
		{
			key = ntohl(*(unsigned int *)T);
			T += 4;
			unsigned int mapsTo = ntohl(*(unsigned int *)T);
			ChildUnit *childUnit = unitDefinition->childUnits.get( key );
			if ( !childUnit )
			{
				childUnit = unitDefinition->childUnits.add( key );
			}
			childUnit->key = mapsTo;
			childUnit->child = 0;
			T += 4;

			D_LOAD(C_Log("key [0x%08X] maps to [0x%08X]", key, mapsTo));
		}
	}

	section->sourceOffset = ntohl( *(unsigned int *)T );
	T += 4;
	if ( !section->sourceOffset )
	{
		D_LOAD(C_Log("source offset [%d]", section->sourceOffset));
	}

	// now that all the units are loaded, set up the children
	CCLinkHash<UnitDefinition>::Iterator iter( C->unitDefinitions );
	for( UnitDefinition* U = iter.getFirst(); U; U = iter.getNext() )
	{
		CCLinkHash<ChildUnit>::Iterator iter2(U->childUnits);
		for( ChildUnit* child = iter2.getFirst(); child; child = iter2.getNext() )
		{
			UnitDefinition* unit = C->unitDefinitions.get( child->key );
			if ( unit )
			{
				child->child = unit;
			}
		}
	}

	return textLocation + sizeof(CodeHeader);
}

//------------------------------------------------------------------------------
int Callisto_runCompiled( Callisto_Context* C,
						  const char* inData,
						  const unsigned int inLen,
						  const Callisto_Handle argumentValueHandle )
{
	int entryPoint = loadEx( C, inData, inLen );
	if ( entryPoint < 0 )
	{
		return entryPoint;
	}
	
	Callisto_ExecutionContext *exec = getExecutionContext( C );
		
	exec->state = Callisto_ThreadState::Runnable;
	exec->textOffsetToResume = entryPoint;

	if ( argumentValueHandle )
	{
		Value* val = getValueFromHandle( C, argumentValueHandle );
		if ( val )
		{
			unsigned int key = hash32( "_argv", 5 );
			Value *arg = C->globalNameSpace->get( key );
			if ( !arg )
			{
				arg = C->globalNameSpace->add( key );
			}
			else
			{
				clearValueOnly( *arg );
			}

			valueMove( arg, val );
		}

		Callisto_releaseValue( C, argumentValueHandle );
	}
	
	return runScheduler( C );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_call( Callisto_ExecutionContext *E, const char* unitName, const Callisto_Handle argumentHandle )
{
	Callisto_Handle args[2] = { argumentHandle, 0 };
	return Callisto_call( E->callisto, unitName, args );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_call( Callisto_Context *C, const char* unitName, const Callisto_Handle argumentHandle )
{
	Callisto_Handle args[2] = { argumentHandle, 0 };
	return Callisto_call( C, unitName, args );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_call( Callisto_Context* C, const char* unitName, const Callisto_Handle* argumentValueHandleList )
{
	Callisto_ExecutionContext *exec = getExecutionContext( C );
	Callisto_Handle ret = Callisto_call( exec, unitName, argumentValueHandleList );
	C->threads.remove( exec->threadId );
	return ret;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_call( Callisto_ExecutionContext *E, const char* unitName, const Callisto_Handle* argumentValueHandleList )
{
	UnitDefinition *U = E->callisto->unitDefinitions.get( hash32(unitName, strlen(unitName)) );
	if ( !U )
	{
		g_Callisto_lastErr = CE_UnitNotFound;
		return 0;
	}

	Value* val;

	E->numberOfArgs = 0;
	if ( argumentValueHandleList )
	{
		for( ; argumentValueHandleList[(int)E->numberOfArgs]; ++E->numberOfArgs )
		{
			if ( (val = getValueFromHandle(E->callisto, argumentValueHandleList[(int)E->numberOfArgs])) == 0 )
			{
				g_Callisto_lastErr = CE_HandleNotFound;
				return 0;
			}

			valueMirror( getNextStackEntry(E), val );
		}
		
		E->numberOfArgs++;
	}

	if ( U->function ) // call right back to c++? okay
	{
		E->Args = E->stack + (E->stackPointer - E->numberOfArgs);
		E->userData = U->userData;

		Callisto_Handle funcReturn = U->function( E );

		popNum( E, E->numberOfArgs );
		
		return funcReturn;
	}
	else
	{
		val = getNextStackEntry( E ); // create a call-frame
		val->type32 = CTYPE_FRAME;
		val->frame = Callisto_ExecutionContext::m_framePool.get();

		val->frame->next = E->frame;
		E->frame = val->frame;

		val->frame->unitContainer.u = U;
		val->frame->unitContainer.unitSpace = Value::m_unitSpacePool.get();
		val->frame->unitContainer.type32 = CTYPE_UNIT;
		val->frame->returnVector = 0; // come back HERE
		val->frame->numberOfArguments = E->numberOfArgs;
		E->textOffsetToResume = U->textOffset;

		execute( E );

		Callisto_Handle ret = setValueSetup( E->callisto, 0, &val );

		val = getValueFromHandle( E->callisto, ret );
		clearValueOnly( *val );
		valueMove( val, E->stack + (E->stackPointer - 1) );

		popNum( E, E->numberOfArgs + 2 ); // args plus frame plus return value

		return ret;
	}
}

//------------------------------------------------------------------------------
const unsigned int Callisto_getNumberOfArgs( Callisto_ExecutionContext* E )
{
	return E->numberOfArgs;
}

//------------------------------------------------------------------------------
const char* Callisto_getStringArg( Callisto_ExecutionContext* E,
								   const int argNum,
								   const char** data,
								   unsigned int* len )
{
	if ( E->numberOfArgs <= argNum )
	{
		return 0;
	}

	const Value *V = (E->Args[argNum].type == CTYPE_REFERENCE) ? E->Args[argNum].v : E->Args + argNum;
	if ( V->type != CTYPE_STRING )
	{
		return 0;
	}

	if ( data )
	{
		*data = V->string->c_str();
	}

	if ( len )
	{
		*len = V->string->length();
	}

	return V->string->c_str();
}

//------------------------------------------------------------------------------
const wchar_t* Callisto_getWStringArg( Callisto_ExecutionContext* E,
									   const int argNum,
									   wchar_t* data,
									   unsigned int* len )
{
	if ( (E->numberOfArgs <= argNum) || !data || !len )
	{
		return 0;
	}

	const Value *V = (E->Args[argNum].type == CTYPE_REFERENCE) ? E->Args[argNum].v : E->Args + argNum;
	if ( V->type != CTYPE_STRING )
	{
		return 0;
	}

	W_str ret;

	UTF8ToUnicode( (unsigned char *)V->string->c_str(), ret );

	if ( ret.size() < *len )
	{
		*len = ret.size();
	}
	
	return wcsncpy( data, ret.c_str(), *len );
}

//------------------------------------------------------------------------------
const char* Callisto_formatArg( Callisto_ExecutionContext* E, const int argNum, char* data, unsigned int* len )
{
	if ( E->numberOfArgs <= argNum )
	{
		return 0;
	}

	const Value *V = (E->Args[argNum].type == CTYPE_REFERENCE) ? E->Args[argNum].v : E->Args + argNum;

	if ( V->type == CTYPE_STRING )
	{
		if ( data && len )
		{
			unsigned int size = (*len > V->string->size()) ? V->string->size() : *len;
			memcpy( data, V->string->c_str(), size );
			data[size] = 0;
			*len = size;
		}

		return V->string->c_str();
	}

	if ( !data || !len )
	{
		return 0;
	}

	switch( V->type )
	{
		default:
		case CTYPE_NULL: data[0] = 0; *len = 0; break; // valid but nothing
		case CTYPE_INT: *len = snprintf( data, *len, "%lld", (long long int)V->i64 ); break;
		case CTYPE_FLOAT: *len = snprintf( data, *len, "%g", V->d ); break;
		case CTYPE_UNIT: *len = snprintf( data, *len, "%s", "<unit>" ); break;
		case CTYPE_TABLE: *len = snprintf( data, *len, "%s", "<table>" ); break;
		case CTYPE_ARRAY: *len = snprintf( data, *len, "%s", "<array>" ); break;
	}

	return data;
}

//------------------------------------------------------------------------------
const int64_t Callisto_getIntArg( Callisto_ExecutionContext* E, const int argNum )
{
	if ( E->numberOfArgs > argNum )
	{
		const Value *V = (E->Args[argNum].type == CTYPE_REFERENCE) ? E->Args[argNum].v : E->Args + argNum;
		if ( V->type == CTYPE_INT )
		{
			return V->i64;
		}
		else if ( V->type == CTYPE_FLOAT )
		{
			return (int64_t)V->d;
		}
	}

	return 0;
}

//------------------------------------------------------------------------------
const double Callisto_getFloatArg( Callisto_ExecutionContext* E, const int argNum )
{
	if ( E->numberOfArgs > argNum )
	{
		const Value *V = (E->Args[argNum].type == CTYPE_REFERENCE) ? E->Args[argNum].v : E->Args + argNum;
		if ( V->type == CTYPE_FLOAT )
		{
			return V->d;
		}
		else if ( V->type == CTYPE_INT )
		{
			return (double)V->i64;
		}
	}

	return 0;
}

//------------------------------------------------------------------------------
void* Callisto_getUserData( Callisto_ExecutionContext* E )
{
	return E->userData;
}

//------------------------------------------------------------------------------
static Callisto_ArgType userTypeFromValueType( const int valueType )
{
	switch( valueType )
	{
		case CTYPE_INT: return Callisto_ArgInt;
		case CTYPE_FLOAT: return Callisto_ArgFloat;
		case CTYPE_STRING: return Callisto_ArgString;
		case CTYPE_UNIT: return Callisto_ArgUnit;
		case CTYPE_TABLE: return Callisto_ArgTable;
		case CTYPE_ARRAY: return Callisto_ArgArray;
		case CTYPE_STRING_ITERATOR: return Callisto_ArgStringIterator;
		case CTYPE_TABLE_ITERATOR: return Callisto_ArgTableIterator;
		case CTYPE_ARRAY_ITERATOR: return Callisto_ArgArrayIterator;
		default: return Callisto_ArgNull;
	}
}

//------------------------------------------------------------------------------
static int valueTypeFromUserType( const int userType )
{
	switch( userType )
	{
		case Callisto_ArgInt: return CTYPE_INT;
		case Callisto_ArgFloat: return CTYPE_FLOAT;
		case Callisto_ArgString: return CTYPE_STRING;
		case Callisto_ArgUnit: return CTYPE_UNIT;
		case Callisto_ArgTable: return CTYPE_TABLE;
		case Callisto_ArgArray: return CTYPE_ARRAY;
		case Callisto_ArgStringIterator: return CTYPE_STRING_ITERATOR;
		case Callisto_ArgTableIterator: return CTYPE_TABLE_ITERATOR;
		case Callisto_ArgArrayIterator: return CTYPE_ARRAY_ITERATOR;
		default: return CTYPE_NULL;
	}
}

//------------------------------------------------------------------------------
const Callisto_ArgType Callisto_getArgType( Callisto_ExecutionContext* E, const int argNum )
{
	if ( E->numberOfArgs > argNum )
	{
		const Value *V = (E->Args[argNum].type == CTYPE_REFERENCE) ? E->Args[argNum].v : E->Args + argNum;
		return userTypeFromValueType( V->type );
	}

	return Callisto_ArgNull;
}

//------------------------------------------------------------------------------
const Callisto_ArgType Callisto_getHandleType( Callisto_ExecutionContext* E, const Callisto_Handle handle )
{
	return Callisto_getHandleType( E->callisto, handle );
}

//------------------------------------------------------------------------------
const Callisto_ArgType Callisto_getHandleType( Callisto_Context* C, const Callisto_Handle handle )
{
	Value *V = getValueFromHandle( C, handle );
	return V ? userTypeFromValueType( V->type ) : Callisto_ArgNull;
}

//------------------------------------------------------------------------------
const int Callisto_handleToString( Callisto_Context* C, const Callisto_Handle handle, const char** buf, int* len )
{
	if ( !buf )
	{
		return CE_NullPointer;
	}

	*buf = 0;
	
	Value *V = getValueFromHandle( C, handle );
	if ( !V )
	{
		return CE_HandleNotFound;
	}
	
	if ( V->type != CTYPE_STRING )
	{
		return CE_IncompatibleDataTypes;
	}

	*buf = V->string->c_str();
	if ( len )
	{
		*len = (int)V->string->size();
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
const int Callisto_handleToString( Callisto_ExecutionContext* E, const Callisto_Handle handle, const char** buf, int* len )
{
	return Callisto_handleToString( E->callisto, handle, buf, len );
}

//------------------------------------------------------------------------------
const int Callisto_handleToFloat( Callisto_Context* C, const Callisto_Handle handle, double* value )
{
	if ( !value )
	{
		return CE_NullPointer;
	}
	
	Value *V = getValueFromHandle( C, handle );
	if ( !V )
	{
		return CE_HandleNotFound;
	}

	if ( V->type == CTYPE_FLOAT )
	{
		*value = V->d;
		return 0;
	}
	else if ( V->type == CTYPE_INT )
	{
		*value = (double)V->i64;
		return 0;
	}

	return CE_IncompatibleDataTypes;
}

//------------------------------------------------------------------------------
const int Callisto_handleToFloat( Callisto_ExecutionContext* E, const Callisto_Handle handle, double* value )
{
	return Callisto_handleToFloat( E->callisto, handle, value );
}

//------------------------------------------------------------------------------
const int Callisto_handleToInt( Callisto_Context* C, const Callisto_Handle handle, int64_t* value )
{
	if ( !value )
	{
		return CE_NullPointer;
	}

	Value *V = getValueFromHandle( C, handle );
	if ( !V )
	{
		return CE_HandleNotFound;
	}

	if ( V->type == CTYPE_FLOAT )
	{
		*value = (int64_t)V->d;
		return 0;
	}
	else if ( V->type == CTYPE_INT )
	{
		*value = V->i64;
		return 0;
	}

	return CE_IncompatibleDataTypes;
}

//------------------------------------------------------------------------------
const int Callisto_handleToInt( Callisto_ExecutionContext* E, const Callisto_Handle handle, int64_t* value )
{
	return Callisto_handleToInt( E->callisto, handle, value );
}

//------------------------------------------------------------------------------
void Callisto_setTypeMethods( Callisto_Context* C, const Callisto_ArgType type, const Callisto_FunctionEntry* entries, const int numberOfEntries )
{
	int vtype = valueTypeFromUserType( type );
	
	CCLinkHash<UnitDefinition>* table = C->typeFunctions.get( vtype );
	if ( !table )
	{
		table = C->typeFunctions.add( vtype );
	}

	for( int i=0; i<numberOfEntries; i++ )
	{
		unsigned int key = hash32( entries[i].functionName, strlen(entries[i].functionName) );
		UnitDefinition* M = table->get( key );
		if ( !M )
		{
			M = table->add( key );
		}

		M->function = entries[i].function;
		M->userData = entries[i].userData;
	}
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_setValue( Callisto_ExecutionContext* E, const int64_t i, const Callisto_Handle handle )
{
	return Callisto_setValue( E->callisto, i, handle );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_setValue( Callisto_ExecutionContext* E, const double d, const Callisto_Handle handle )
{
	return Callisto_setValue( E->callisto, d, handle );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_setValue( Callisto_ExecutionContext* E, const char* string, const int len, const Callisto_Handle handle )
{
	return Callisto_setValue( E->callisto, string, len, handle );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_setValue( Callisto_ExecutionContext* E, const wchar_t* wstring, const int len, const Callisto_Handle handle )
{
	return Callisto_setValue( E->callisto, wstring, len, handle );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_setValue( Callisto_Context* C, const int64_t i, const Callisto_Handle handle )
{
	Value* val;
	Callisto_Handle ret = setValueSetup( C, handle, &val );
	val->type32 = CTYPE_INT;
	val->i64 = i;
	return ret;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_setValue( Callisto_Context* C, const double d, const Callisto_Handle handle )
{
	Value* val;
	Callisto_Handle ret = setValueSetup( C, handle, &val );
	val->type32 = CTYPE_FLOAT;
	val->d = d;
	return ret;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_setValue( Callisto_Context* C, const char* string, const int len, const Callisto_Handle handle )
{
	Value* val;
	Callisto_Handle ret = setValueSetup( C, handle, &val );
	val->allocString();
	val->string->set( string, len );
	return ret;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_setValue( Callisto_Context* C, const wchar_t* wstring, const int len, const Callisto_Handle handle )
{
	Value* val;
	Callisto_Handle ret = setValueSetup( C, handle, &val );
	val->allocString();
	unicodeToUTF8( wstring, len, *val->string );
	return ret;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createArray( Callisto_ExecutionContext* E, const Callisto_Handle handle )
{
	return Callisto_createArray( E->callisto, handle );
}

//------------------------------------------------------------------------------
const int Callisto_setArrayValue( Callisto_ExecutionContext* E, const Callisto_Handle array, const Callisto_Handle value, const int index )
{
	return Callisto_setArrayValue( E->callisto, array, value, index );
}

//------------------------------------------------------------------------------
const int Callisto_getArrayCount( Callisto_ExecutionContext* E, const Callisto_Handle array )
{
	return Callisto_getArrayCount( E->callisto, array );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_getArrayValue( Callisto_ExecutionContext* E, const Callisto_Handle array, const int index )
{
	return Callisto_getArrayValue( E->callisto, array, index );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createTable( Callisto_ExecutionContext* E, const Callisto_Handle handle )
{
	return Callisto_createTable( E->callisto, handle );
}

//------------------------------------------------------------------------------
const int Callisto_setTableValue( Callisto_ExecutionContext* E, const Callisto_Handle table, const Callisto_Handle key, const Callisto_Handle value )
{
	return Callisto_setTableValue( E->callisto, table, key, value );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_getTableValue( Callisto_ExecutionContext* E, const Callisto_Handle table, const Callisto_Handle key )
{
	return Callisto_getTableValue( E->callisto, table, key );
}

//------------------------------------------------------------------------------
const int Callisto_getTableCount( Callisto_ExecutionContext* E, const Callisto_Handle table )
{
	return Callisto_getTableCount( E->callisto, table );
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createArray( Callisto_Context* C, const Callisto_Handle handle )
{
	Value* val;
	Callisto_Handle ret = setValueSetup( C, handle, &val );
	val->allocArray();
	return ret;
}

//------------------------------------------------------------------------------
const int Callisto_setArrayValue( Callisto_Context* C, const Callisto_Handle array, const Callisto_Handle value, const int index )
{
	Value* val = C->globalNameSpace->get( array );
	if ( !val )
	{
		return -1;
	}
	
	if ( val->type != CTYPE_ARRAY )
	{
		clearValue( *val );
		val->allocArray();
	}

	int insertIndex = (index >= 0) ? index : val->array->count();
	Value& target = (*val->array)[ insertIndex ];
	
	val = C->globalNameSpace->get( value );
	if ( val )
	{
		clearValueOnly( target );
		valueMove( &target, val );
		Callisto_releaseValue( C, value );
	}
	else
	{
		clearValue( target );
	}

	return insertIndex;
}

//------------------------------------------------------------------------------
const int Callisto_getArrayCount( Callisto_Context* C, const Callisto_Handle array )
{
	Value* V = C->globalNameSpace->get( array );
	return (V && (V->type == CTYPE_ARRAY)) ? V->array->count() : 0;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_getArrayValue( Callisto_Context* C, const Callisto_Handle array, const int index )
{
	// how to give the user access to this value?
	return 0;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createTable( Callisto_Context* C, const Callisto_Handle handle )
{
	Value* val;
	Callisto_Handle ret = setValueSetup( C, handle, &val );
	val->allocTable();
	return ret;
}

//------------------------------------------------------------------------------
const int Callisto_getTableCount( Callisto_Context* C, const Callisto_Handle table )
{
	Value* V = C->globalNameSpace->get( table );
	return (V && (V->type == CTYPE_TABLE)) ? V->tableSpace->count() : 0;
}

//------------------------------------------------------------------------------
const int Callisto_setTableValue( Callisto_Context* C, const Callisto_Handle table, const Callisto_Handle key, const Callisto_Handle value )
{
	Value* tableHandle = C->globalNameSpace->get( table );
	if ( !tableHandle )
	{
		return CE_HandleNotFound;
	}
	
	if ( tableHandle->type != CTYPE_TABLE )
	{
		clearValue( *tableHandle );
		tableHandle->allocTable();
	}

	Value* kval = C->globalNameSpace->get( key );
	if ( !kval )
	{
		return CE_HandleNotFound;
	}

	if ( !(kval->type & BIT_CAN_HASH) )
	{
		return CE_UnhashableDataType;
	}

	Value* vval = C->globalNameSpace->get( value );
	if ( !vval )
	{
		return CE_HandleNotFound;
	}

	unsigned int hash = (unsigned int)kval->getHash();
	CKeyValue* tableEntry = tableHandle->tableSpace->add( hash );

	clearValueOnly( tableEntry->key );
	clearValueOnly( tableEntry->value );
	
	valueMirror( &tableEntry->key, kval );
	valueMirror( &tableEntry->value, vval );
	
	Callisto_releaseValue( C, key );
	Callisto_releaseValue( C, value );

	return 0;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_getTableValue( Callisto_Context* C, const Callisto_Handle table, const Callisto_Handle key )
{
	// how to represent the value to the user?
	return 0;
}

//------------------------------------------------------------------------------
void Callisto_releaseValue( Callisto_ExecutionContext* E, const Callisto_Handle handle )
{
	return Callisto_releaseValue( E->callisto, handle );
}

//------------------------------------------------------------------------------
void Callisto_releaseValue( Callisto_Context* C, const Callisto_Handle handle )
{
	C->globalNameSpace->remove( handle ); // deleting it will call the destructor which does the rest
}

//------------------------------------------------------------------------------
int Callisto_importConstant( Callisto_ExecutionContext* E, const char* name, const Callisto_Handle handle )
{
	return Callisto_importConstant( E->callisto, name, handle );
}

//------------------------------------------------------------------------------
int Callisto_importConstant( Callisto_Context* C, const char* name, const Callisto_Handle handle )
{
	unsigned int key = hash32( name, strlen(name) );

	Value* hval = getValueFromHandle( C, handle );
	if ( !hval )
	{
		return CE_HandleNotFound;
	}

	Value* V = C->globalNameSpace->get( key );
	if ( !V )
	{
		V = C->globalNameSpace->add( key );
	}
	else
	{
		clearValueOnly( *V );
	}

	valueMove( V, hval );
	Callisto_releaseValue( C, handle );

	return 0;
}

//------------------------------------------------------------------------------
const Callisto_ThreadState Callisto_getThreadState( Callisto_ExecutionContext* E )
{
	return (Callisto_ThreadState)E->state;
}

//------------------------------------------------------------------------------
const int Callisto_getThreadId( Callisto_ExecutionContext* E )
{
	return E->threadId;
}

//------------------------------------------------------------------------------
void Callisto_yield( Callisto_ExecutionContext* E )
{
	E->state |= Callisto_ThreadState::Yield;
}

//------------------------------------------------------------------------------
const int Callisto_startThread( Callisto_ExecutionContext* E, const char* unitName, const Callisto_Handle argumentHandle )
{
	Callisto_Handle args[2] = { argumentHandle, 0 };
	return Callisto_startThread( E->callisto, unitName, args );
}

//------------------------------------------------------------------------------
const int Callisto_startThread( Callisto_ExecutionContext* E, const char* unitName, const Callisto_Handle* argumentValueHandleList )
{
	return Callisto_startThread( E->callisto, unitName, argumentValueHandleList );
}

//------------------------------------------------------------------------------
const int Callisto_startThread( Callisto_Context* C, const char* unitName, const Callisto_Handle argumentHandle )
{
	Callisto_Handle args[2] = { argumentHandle, 0 };
	return Callisto_startThread( C, unitName, args );
}

//------------------------------------------------------------------------------
const int Callisto_startThread( Callisto_Context* C, const char* unitName, const Callisto_Handle* argumentValueHandleList )
{
	UnitDefinition *unit = C->unitDefinitions.get( hash32(unitName, strlen(unitName)) );
	if ( !unit )
	{
		g_Callisto_lastErr = CE_UnitNotFound;
		return 0;
	}

	int args = 0;
	for( ; argumentValueHandleList && argumentValueHandleList[args]; ++args )
	{
		if ( !getValueFromHandle(C, argumentValueHandleList[args]) )
		{
			g_Callisto_lastErr = CE_HandleNotFound;
			return 0;
		}
	}

	Callisto_ExecutionContext* newExec = doCreateThreadFromUnit( C, unit, args );

	for( int i=0; i<args; ++i)
	{
		valueMirror( newExec->stack + newExec->stackPointer - (i+2),
					 getValueFromHandle(C, argumentValueHandleList[args]) );
	}

	return newExec->threadId;
}
