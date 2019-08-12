#include "util.h"
#include "arch.h"

extern CLog Log;

int g_Callisto_lastErr = CE_NoError;

#define D_LOAD(a) //a
#define D_EXECUTE(a) //a
#define D_IMPORT(a) //a

//------------------------------------------------------------------------------
Callisto_Context* Callisto_createContext()
{
	return new Callisto_Context;
}

//------------------------------------------------------------------------------
Callisto_ExecutionContext* Callisto_createExecutionContext( Callisto_Context* C )
{
	int thread = 0;
	while( C->threads.get( thread = ++C->threadIdGenerator ) );

	Callisto_ExecutionContext *context = C->threads.get( thread );
	if ( !context )
	{
		context = C->threads.add( thread );

		context->pos = 0;
		context->stackPointer = 0;
		context->stackSize = 32;

		context->stack = new Value[context->stackSize];

		context->thread = thread;
		context->callisto = C;
	}

	return context;
}

//------------------------------------------------------------------------------
void Callisto_destroyContext( Callisto_Context* C )
{
	delete C;
}

//------------------------------------------------------------------------------
void Callisto_destroyExecutionContext( Callisto_ExecutionContext* E )
{
	E->callisto->threads.remove( E->thread );
}

//------------------------------------------------------------------------------
const int Callisto_lastErr()
{
	return g_Callisto_lastErr;
}

//------------------------------------------------------------------------------
void Callisto_importList( Callisto_Context* C, const Callisto_FunctionEntry* entries, const int numberOfEntries )
{
	for( int i=0; i<numberOfEntries; i++ )
	{
		unsigned int key = hashStr(entries[i].label);
		Unit *U = C->unitList.get( key );
		if ( !U )
		{
			U = C->unitList.add( key );
		}

		U->function = entries[i].function;
		U->userData = entries[i].userData;
		U->textOffset = 0;

		C->globalUnit.childUnits.add( key )->child = U; // cache here so it can be found in global space too

		D_IMPORT(Log("imported function [0x%08X]", key));
	}
}

//------------------------------------------------------------------------------
void Callisto_importFunction( Callisto_Context* C, const char* label, Callisto_CallbackFunction function, void* userData )
{
	Callisto_FunctionEntry entry;
	entry.label = label;
	entry.function = function;
	entry.userData = userData;
	Callisto_importList( C, &entry, 1 );
}

//------------------------------------------------------------------------------
const char* Callisto_formatError( int err )
{
	static Cstr ret;
	switch( err )
	{
		case CE_NoError: ret = "CE_NoError"; break;
		case CE_FileErr: ret = "CE_FileErr: error reading/writing/opening a file "; break;
		case CE_DataErr: ret = "CE_DataErr: data format was invalid, usually a null pointer or zero length"; break;
		case CE_BadVersion: ret = "CE_BadVersion: callisto bytecode version not recognized"; break;
		case CE_ThreadNotFound: ret = "CE_ThreadNotFound: execution thread specified has not been created"; break;
		case CE_UnitNotFound: ret = "CE_UnitNotFound: unit function requested was not located in the loaded byte code"; break;
		case CE_ParentNotFound: ret = "CE_ParentNotFound: RUNTIME unit parent not found in the bytecode symbols for a 'new' operation"; break;
		case CE_IncompatibleDataTypes: ret = "CE_IncompatibleDataTypes: operation attempted between two data types that don't play together"; break;
		case CE_StackedPostOperatorsUnsupported: ret = "CE_StackedPostOperatorsUnsupported: can't go i++ ++ ++; it parses but it shouldn't"; break;
		case CE_AssignedValueToStack: ret = "CE_AssignedValueToStack: not suppsoed to be able to generate this code, but well done!"; break;
		case CE_UnhashableDataType: ret = "CE_UnhashableDataType: type cannot be used in a switch or anywhere else a hash is called for"; break;
		case CE_TryingToAccessMembersOfNonUnit: ret = "CE_TryingToAccessMembersOfNonUnit: using the '.' operator on a type that isn't a unit"; break;
		case CE_MemberNotFound: ret = "CE_MemberNotFound: child unit you are trying to call was not declared"; break;
		case CE_TypeCannotBeIterated: ret = "CE_TypeCannotBeIterated: called 'foreach' on a type that does not support it"; break;
		case CE_UnrecognizedByteCode: ret = "CE_UnrecognizedByteCode: a byte code was read that is not recognized, probably corrupt file"; break;
		case CE_CannotOperateOnLiteral: ret = "CE_CannotOperateOnLiteral: this operation cannot be performed on a literal value"; break;
		case CE_HandleNotFound: ret = "CE_HandleNotFound: the supplied handle had not been created"; break;
	}
	return ret.c_str();
}

//------------------------------------------------------------------------------
int pushCommandLine( Callisto_ExecutionContext* context, const Callisto_Handle argumentValueHandle )
{
	unsigned int commandLine = hashStr( "_argv" );
	Value* arg = context->currentUnit->refLocalSpace->item.get( commandLine );
	if ( !arg )
	{
		arg = context->currentUnit->refLocalSpace->item.add( commandLine );
	}
	
	if ( argumentValueHandle )
	{
		Value *V = context->callisto->global.refLocalSpace->item.get( argumentValueHandle );
		if ( !V )
		{
			return g_Callisto_lastErr = CE_HandleNotFound;
		}
		
		valueDeepCopy( arg, V );
	}

	return CE_NoError;
}

//------------------------------------------------------------------------------
int Callisto_loadFile( Callisto_ExecutionContext* E, const char* fileName, const Callisto_Handle argumentValueHandle )
{
	Cstr file;
	if ( !file.fileToBuffer(fileName) )
	{
		return g_Callisto_lastErr = CE_FileErr;
	}

	return Callisto_load( E, file.c_str(), file.size(), argumentValueHandle );
}

//------------------------------------------------------------------------------
int Callisto_load( Callisto_ExecutionContext* E, const char* inData, const unsigned int inLen, const Callisto_Handle argumentValueHandle )
{
	if ( !inData )
	{
		return g_Callisto_lastErr = CE_DataErr;
	}

	char* data = const_cast<char*>(inData);
	unsigned int len = inLen ? inLen : strlen(inData);

	CodeHeader *header = (CodeHeader *)data;

	if ( ((len < sizeof(CodeHeader)) || (header->CRC != hash(header, sizeof(CodeHeader) - 4)))
		 && Callisto_parse(inData, len, &data, &len) )
	{
		return g_Callisto_lastErr = CE_DataErr;
	}

	E->callisto->text.set( data, len );
	
	header = (CodeHeader *)data;
	
	if ( header->version != CALLISTO_VERSION )
	{
		D_LOAD(Log("bad version [0x%08X]", header->version));
		return g_Callisto_lastErr = CE_BadVersion;
	}

	D_LOAD(Log("first block [0x%08X]", header->firstUnitBlock));

	int stackPointerIn = E->stackPointer;
	
	E->pos = header->firstUnitBlock;

	// "dummy" unit representing global space
	E->callisto->globalUnit.numberOfParents = 0;
	E->callisto->globalUnit.textOffset = sizeof(CodeHeader);
	// TODO- clear only the NON-imported portions of the context
	
	E->currentUnit = &E->callisto->global; // default global unit
	E->searchChildren = &E->callisto->globalUnit.childUnits;

	E->callisto->symbols.clear();
	for( int i=0; i<header->symbols; i++ )
	{
		unsigned int key = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
		E->pos += 4;
		Cstr* label = E->callisto->symbols.add( key );
		char size = E->callisto->text[E->pos++];
		label->set( E->callisto->text.c_str(E->pos), size );
		E->pos += size;

		D_LOAD(Log("loaded symbol [%s:0x%08X]", label->c_str(), key));
	}

	for( int i=0; i<header->unitCount; i++ )
	{
		Cstr name;
		readStringFromCodeStream( E, name );
		
		unsigned int key = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
		E->pos += 4;
		
		Unit* U = E->callisto->unitList.get( key );
		if ( !U )
		{
			D_LOAD(Log("Adding unit [%s] signature [0x%08X]", name.c_str(), key ));
			U = E->callisto->unitList.add( key );
			E->callisto->globalUnit.childUnits.add( key )->child = U; // cache here so it can be found in global space too
		}
		
		U->textOffset = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
		E->pos += 4;
		U->function = 0;

		U->numberOfParents = (int)E->callisto->text[E->pos++];

		D_LOAD(Log("unit [%s]:[0x%08X] [%d]parents", name.c_str(), U->textOffset, U->numberOfParents));

		for( int i=0; i<U->numberOfParents; i++ )
		{
			U->parentList[i] = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
			D_LOAD(Log("parent[0x%08X]", U->parentList[i]));
			E->pos += 4;
		}

		int children = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
		E->pos += 4;

		D_LOAD(Log("%d children", children));
		for( int i=0; i<children; i++ )
		{
			unsigned int key = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
			E->pos += 4;
			unsigned int mapsTo = ntohl(*(unsigned int *)E->callisto->text.c_str( E->pos ));
			ChildUnit *childUnit = U->childUnits.add(key);
			childUnit->key = mapsTo;
			childUnit->child = 0;
			E->pos += 4;

			D_LOAD(Log("key [0x%08X] maps to [0x%08X]", key, mapsTo));
		}
	}

	if ( pushCommandLine(E, argumentValueHandle) )
	{
		return g_Callisto_lastErr;
	}
	
	g_Callisto_lastErr = execute( E, sizeof(CodeHeader), 0 );

	if ( g_Callisto_lastErr )
	{
		Log( "LOAD ERR: %d:%s [%s]", g_Callisto_lastErr, E->callisto->err.c_str(), Callisto_formatError(g_Callisto_lastErr) );
	}

	popOne( E );

	if ( stackPointerIn != E->stackPointer )
	{
		return g_Callisto_lastErr = CE_DataErr;
	}

	return g_Callisto_lastErr;
}

//------------------------------------------------------------------------------
int Callisto_execute( Callisto_ExecutionContext *E, const char* unitName, const unsigned int argumentValueHandle /*=0*/ )
{
	if ( !unitName )
	{
		D_EXECUTE(Log("exec <2>"));
		return g_Callisto_lastErr = CE_UnitNotFound;
	}

	D_EXECUTE(Log("executing [%s]", unitName ));

	ChildUnit* U = E->callisto->globalUnit.childUnits.get( hashStr(unitName) );

	if ( !U )
	{
		return g_Callisto_lastErr = CE_UnitNotFound;
	}
	
	if ( !U->child )
	{
		U->child = E->callisto->unitList.get( U->key );
		if ( !U->child )
		{
			return g_Callisto_lastErr = CE_MemberNotFound;
		}
	}

	CHashTable<ChildUnit>* oldChildren = E->searchChildren;
	E->searchChildren = &U->child->childUnits;

	pushCommandLine( E, argumentValueHandle );
	g_Callisto_lastErr = execute( E, U->child->textOffset, 0 );

	E->searchChildren = oldChildren;

	popOne( E ); // TODO- return value?
	
	return g_Callisto_lastErr;
}

//------------------------------------------------------------------------------
const char* Callisto_getStringArg( Callisto_ExecutionContext* E, const int argNum, char* data, unsigned int* len )
{
	const Value *V = (E->Args[argNum].type == CTYPE_OFFSTACK_REFERENCE) ? E->Args[argNum].v : E->Args + argNum;

	if ( V->type == CTYPE_STRING )
	{
		if ( data && len )
		{
			if ( V->refCstr->item.size() + 1 > *len )
			{
				memcpy( data, V->refCstr->item.c_str(), *len );
			}
			else
			{
				memcpy( data, V->refCstr->item.c_str(), V->refCstr->item.size() + 1 );
				*len = V->refCstr->item.size();
			}

			data[*len] = 0;
		}

		return V->refCstr->item.c_str();
	}
	else if ( data && len && (*len > 0) )
	{
		switch( V->type )
		{
			default:
			case CTYPE_NULL: data[0] = 0; return 0;
			case CTYPE_INT: *len = snprintf( data, *len,"%lld", (long long int)V->i64 ); return data;
			case CTYPE_FLOAT: *len = snprintf( data, *len, "%g", V->d ); return data;
			case CTYPE_WSTRING: *len = snprintf( data, *len, "%S", V->refWstr->item.c_str() ); return data;
			case CTYPE_UNIT: *len = snprintf( data, *len, "%s", "<unit>" ); return data;
			case CTYPE_TABLE: *len = snprintf( data, *len, "%s", "<table>" ); return data;
			case CTYPE_ARRAY: *len = snprintf( data, *len, "%s", "<array>" ); return data;
		}
	}

	return 0;
}

//------------------------------------------------------------------------------
int64_t Callisto_getIntArg( Callisto_ExecutionContext* F, const int argNum )
{
	if ( Callisto_getArgType(F, argNum) == CTYPE_INT )
	{
		return F->Args[argNum].i64;
	}
	else if ( Callisto_getArgType(F, argNum) == CTYPE_FLOAT )
	{
		return (int64_t)F->Args[argNum].d;
	}

	return 0;
}

//------------------------------------------------------------------------------
double Callisto_getFloatArg( Callisto_ExecutionContext* F, const int argNum )
{
	if ( Callisto_getArgType(F, argNum) == CTYPE_INT )
	{
		return (double)F->Args[argNum].i64;
	}
	else if ( Callisto_getArgType(F, argNum) == CTYPE_FLOAT )
	{
		return F->Args[argNum].d;
	}

	return 0;
}

//------------------------------------------------------------------------------
int Callisto_getArgType( Callisto_ExecutionContext* F, const int argNum )
{
	if ( argNum >= F->numberOfArgs )
	{
		return CTYPE_NULL;
	}
		
	return (F->Args[argNum].type == CTYPE_OFFSTACK_REFERENCE) ? F->Args[argNum].v->type : F->Args[argNum].type;
}

//------------------------------------------------------------------------------
void* Callisto_getUserData( Callisto_ExecutionContext* F )
{
	return F->userData;
}

//------------------------------------------------------------------------------
const unsigned int Callisto_getNumberOfArgs( Callisto_ExecutionContext* F )
{
	return F->numberOfArgs;
}

//------------------------------------------------------------------------------
void Callisto_setGlobalMetaTable( Callisto_Context* C, const int type, const Callisto_FunctionEntry* entries, const int numberOfEntries )
{
	CHashTable<MetaTableEntry>* table = C->globalUnit.metaTableByType.get( type );
	if ( !table )
	{
		table = C->globalUnit.metaTableByType.add( type );
	}

	for( int i=0; i<numberOfEntries; i++ )
	{
		unsigned int key = hashStr(entries[i].label);
		MetaTableEntry* M = table->get( key );
		if ( !M )
		{
			M = table->add( key );
		}

//		M->unitKey = 0;
		M->function = entries[i].function;
		M->userData = entries[i].userData;
	}
}

//------------------------------------------------------------------------------
const inline Callisto_Handle getUniqueHandle( Callisto_Context* C )
{
	static Callisto_Handle handle = 0;
	while( C->global.refLocalSpace->item.get( ++handle ) );
	return handle;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E )
{
	const Callisto_Handle handle = getUniqueHandle( E->callisto );
	E->callisto->global.refLocalSpace->item.add( handle );
	return handle;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E, const int64_t i )
{
	const Callisto_Handle handle = getUniqueHandle( E->callisto );
	Value* V = E->callisto->global.refLocalSpace->item.add( handle );
	V->type = CTYPE_INT;
	V->i64 = i;
	return handle;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E, const double d )
{
	const Callisto_Handle handle = getUniqueHandle( E->callisto );
	Value* V = E->callisto->global.refLocalSpace->item.add( handle );
	V->type = CTYPE_FLOAT;
	V->d = d;
	return handle;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E, const char* string, const int len )
{
	const Callisto_Handle handle = getUniqueHandle( E->callisto );
	Value* V = E->callisto->global.refLocalSpace->item.add( handle );
	V->type = CTYPE_STRING;
	V->refCstr = new RefCountedVariable<Cstr>();
	V->refCstr->item.set( string, len ? len : (unsigned int)strlen(string) );
	return handle;
}

//------------------------------------------------------------------------------
const Callisto_Handle Callisto_createValue( Callisto_ExecutionContext* E, const wchar_t* string, const int len )
{
	const Callisto_Handle handle = getUniqueHandle( E->callisto );
	Value* V = E->callisto->global.refLocalSpace->item.add( handle );
	V->type = CTYPE_WSTRING;
	V->refWstr = new RefCountedVariable<Wstr>();
	V->refWstr->item.set( string, len ? len : (unsigned int)wcslen(string) );
	return handle;
}

//------------------------------------------------------------------------------
int Callisto_setValue( Callisto_ExecutionContext* E, const Callisto_Handle loadToHandle, const int64_t i )
{
	Value* V = E->callisto->global.refLocalSpace->item.get( loadToHandle );
	if ( !V )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}
	clearValue( *V );
	
	V->type = CTYPE_INT;
	V->i64 = i;
	
	return CE_NoError;
}

//------------------------------------------------------------------------------
int Callisto_loadValue( Callisto_ExecutionContext* E, const Callisto_Handle loadToHandle, const double d )
{
	Value* V = E->callisto->global.refLocalSpace->item.get( loadToHandle );
	if ( !V )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}
	clearValue( *V );
	
	V->type = CTYPE_FLOAT;
	V->d = d;

	return CE_NoError;
}

//------------------------------------------------------------------------------
int Callisto_loadValue( Callisto_ExecutionContext* E, const Callisto_Handle loadToHandle, const char* string, const int len )
{
	Value* V = E->callisto->global.refLocalSpace->item.get( loadToHandle );
	if ( !V )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}
	clearValue( *V );

	V->type = CTYPE_STRING;
	V->refCstr = new RefCountedVariable<Cstr>();
	V->refCstr->item.set( string, len );

	return CE_NoError;
}

//------------------------------------------------------------------------------
int Callisto_loadValue( Callisto_ExecutionContext* E, const Callisto_Handle loadToHandle, const wchar_t* string, const int len )
{
	Value* V = E->callisto->global.refLocalSpace->item.get( loadToHandle );
	if ( !V )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}
	clearValue( *V );

	V->type = CTYPE_WSTRING;
	V->refWstr = new RefCountedVariable<Wstr>();
	V->refWstr->item.set( string, len );

	return CE_NoError;
}

//------------------------------------------------------------------------------
int Callisto_setArrayValue( Callisto_ExecutionContext* E, const Callisto_Handle handleOfArray, const int index, const Callisto_Handle handleOfData )
{
	Value* array = E->callisto->global.refLocalSpace->item.get( handleOfArray );
	if ( !array )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}

	Value* data = E->callisto->global.refLocalSpace->item.get( handleOfData );
	if ( !data )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}

	if ( array->type != CTYPE_ARRAY )
	{
		clearValue( *array );
		array->type = CTYPE_ARRAY;
		array->refArray = new RefCountedVariable<Carray>();
	}

	Value& target = array->refArray->item[index];
	valueMove( &target, data );
	E->callisto->global.refLocalSpace->item.remove( handleOfData );
	
	return 0;
}

//------------------------------------------------------------------------------
int Callisto_addArrayValue( Callisto_ExecutionContext* E, const Callisto_Handle handleOfArray, const Callisto_Handle handleOfData )
{
	Value* array = E->callisto->global.refLocalSpace->item.get( handleOfArray );
	if ( !array )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}

	Value* data = E->callisto->global.refLocalSpace->item.get( handleOfData );
	if ( !data )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}

	if ( array->type != CTYPE_ARRAY )
	{
		clearValue( *array );
		array->type = CTYPE_ARRAY;
		array->refArray = new RefCountedVariable<Carray>();
	}

	Value& target = array->refArray->item.append();
	valueMove( &target, data );
	E->callisto->global.refLocalSpace->item.remove( handleOfData );

	return array->refArray->item.count() - 1;
}

//------------------------------------------------------------------------------
int Callisto_getArrayCount( Callisto_ExecutionContext* E, const Callisto_Handle handleOfArray )
{
	Value* array = E->callisto->global.refLocalSpace->item.get( handleOfArray );
	if ( !array )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}

	return array->refArray->item.count();
}

//------------------------------------------------------------------------------
int Callisto_setTableValue( Callisto_ExecutionContext* E, const Callisto_Handle handleOfTable, const Callisto_Handle handleOfKey, const Callisto_Handle handleOfData )
{
	Value* table = E->callisto->global.refLocalSpace->item.get( handleOfTable );
	if ( !table )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}

	Value* key = E->callisto->global.refLocalSpace->item.get( handleOfKey );
	if ( !key )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}

	if ( !(key->type & BIT_CAN_HASH) )
	{
		return g_Callisto_lastErr = CE_UnhashableDataType;
	}

	unsigned int hash = key->getHash();
	
	Value* data = E->callisto->global.refLocalSpace->item.get( handleOfData );
	if ( !data )
	{
		return g_Callisto_lastErr = CE_HandleNotFound;
	}

	if ( table->type != CTYPE_TABLE )
	{
		clearValue( *table );
		table->type = CTYPE_TABLE;
		table->refLocalSpace = new RefCountedVariable<CLinkHash<Value>>();
	}

	Value* V = table->refLocalSpace->item.get( hash );
	if ( !V )
	{
		V = table->refLocalSpace->item.add( hash );
	}

	valueMove( V, data );
	E->callisto->global.refLocalSpace->item.remove( handleOfData );
	
	return 0;
}

//------------------------------------------------------------------------------
void Callisto_releaseValue( Callisto_ExecutionContext* E, const Callisto_Handle handle )
{
	E->callisto->global.refLocalSpace->item.remove( handle ); // deleting it will call the destructor which does the rest
}

