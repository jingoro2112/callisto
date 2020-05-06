#include "../include/callisto.h"
#include "vm.h"

//------------------------------------------------------------------------------
Callisto_Handle Callisto_arrayIFuncRemoveCurrent( Callisto_ExecutionContext* E )
{
	int number = (int)Callisto_getIntArg( E, 0 );
	E->object.array->remove( (unsigned int)E->object.iterator->index, number > 0 ? number : 1 );
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_arrayIFuncReset( Callisto_ExecutionContext* E )
{
	E->object.iterator->index = 0;
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_arrayFuncRemove( Callisto_ExecutionContext* E )
{
	int arg = (int)Callisto_getIntArg( E, 0 );
	int number = (int)Callisto_getIntArg( E, 1 );
	E->object.array->remove( arg, number > 0 ? number : 1 );
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_arrayFuncAdd( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs )
	{
		valueMirror( &E->object.array->add(), E->Args );
	}
	
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_arrayFuncInsert( Callisto_ExecutionContext* E )
{
	unsigned int index = -1;
	if ( E->numberOfArgs >= 2 )
	{
		index = (unsigned int)Callisto_getIntArg( E, 1 );
		valueMirror( &E->object.array->add(index), E->Args );
	}

	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_arrayFuncCount( Callisto_ExecutionContext* E )
{
	return Callisto_setValue( E, (int64_t)E->object.array->count() );
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_static_arrayFuncRemove( Callisto_ExecutionContext* E )
{
	Value* array = getValueFromArg( E, 0 );
	if ( array && array->type == CTYPE_ARRAY )
	{
		int arg = (int)Callisto_getIntArg( E, 1 );
		int number = (int)Callisto_getIntArg( E, 2 );
		E->object.array->remove( arg, number > 0 ? number : 1 );
		return Callisto_THIS_HANDLE;
	}
	else
	{
		return Callisto_NULL_HANDLE;
	}
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_static_arrayFuncAdd( Callisto_ExecutionContext* E )
{
	Value* array = getValueFromArg( E, 0 );
	Value* item = getValueFromArg( E, 1 );
	if ( array && item && array->type == CTYPE_ARRAY )
	{
		deepCopy( &array->array->add(), item );
		return Callisto_THIS_HANDLE;
	}
	else
	{
		return Callisto_NULL_HANDLE;
	}
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_static_arrayFuncCount( Callisto_ExecutionContext* E )
{
	Value* array = getValueFromArg( E, 0 );
	return (array && array->type == CTYPE_ARRAY) ? Callisto_setValue(E, (int64_t)array->array->count()) : Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_static_arrayFuncInsert( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs >= 3 )
	{
		Value* array = getValueFromArg( E, 0 );
		if ( array->type == CTYPE_ARRAY )
		{
			unsigned int index = (unsigned int)Callisto_getIntArg( E, 1 );
			{
				valueMirror( &array->array->add(index), getValueFromArg( E, 2) );
			}
		}
	}

	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_tableIFuncReset( Callisto_ExecutionContext* E )
{
	E->object.iterator->iterator.reset();
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_tableIFuncRemoveCurrent( Callisto_ExecutionContext* E )
{
	E->object.iterator->iterator.removeCurrent();
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_tableFuncRemove( Callisto_ExecutionContext* E )
{
	E->object.iterator->iterator.removeCurrent();
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_tableFuncInsert( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs >= 2 )
	{
		Value* key = getValueFromArg( E, 0 );
		if ( key->type & BIT_CAN_HASH )
		{
			int hash = key->getHash();
			CKeyValue* KV = E->object.tableSpace->get( hash );
			if ( !KV )
			{
				KV = E->object.tableSpace->add( hash );
			}
			else
			{
				clearValue( KV->value );
				clearValue( KV->key );
			}

			valueMirror( &KV->value, getValueFromArg(E, 1) );
			valueMirror( &KV->key, key );
		}
	}

	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_tableFuncCount( Callisto_ExecutionContext* E )
{
	return Callisto_setValue( E, (int64_t)E->object.tableSpace->count() );
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_static_tableFuncRemove( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs )
	{
		Value* key = getValueFromArg( E, 0 );
		if ( key->type & BIT_CAN_HASH )
		{
			E->object.tableSpace->remove( key->getHash() );
		}
	}
	
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_static_tableFuncInsert( Callisto_ExecutionContext* E )
{
	if ( E->numberOfArgs >= 3 )
	{
		Value* table = getValueFromArg( E, 0 );
		if ( table->type == CTYPE_TABLE )
		{
			Value* key = getValueFromArg( E, 1 );
			if ( key->type & BIT_CAN_HASH )
			{
				int hash = key->getHash();
				CKeyValue* KV = E->object.tableSpace->get( hash );
				if ( !KV )
				{
					KV = E->object.tableSpace->add( hash );
				}
				else
				{
					clearValue( KV->value );
					clearValue( KV->key );
				}

				valueMirror( &KV->value, getValueFromArg(E, 2) );
				valueMirror( &KV->key, key );
			}
		}
	}

	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle Callisto_static_tableFuncCount( Callisto_ExecutionContext* E )
{
	Value* table = getValueFromArg( E, 0 );
	if ( table && table->type == CTYPE_TABLE )
	{
		return Callisto_setValue( E, (int64_t)table->tableSpace->count() );
	}
	
	return 0;
}

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry Callisto_arrayIteratorFuncs[]=
{
	{ "removeCurrent", Callisto_arrayIFuncRemoveCurrent, 0 },
	{ "remove", Callisto_arrayFuncRemove, 0 },
	{ "add", Callisto_arrayFuncAdd, 0 },
	{ "insert", Callisto_arrayFuncInsert, 0 },
	{ "reset", Callisto_arrayIFuncReset, 0 },
	{ "count", Callisto_arrayFuncCount, 0 },
};

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry Callisto_arrayFuncs[]=
{
	{ "remove", Callisto_arrayFuncRemove, 0 },
	{ "insert", Callisto_arrayFuncInsert, 0 },
	{ "add", Callisto_arrayFuncAdd, 0 },
	{ "count", Callisto_arrayFuncCount, 0 },
};

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry Callisto_static_arrayFuncs[]=
{
	{ "array::remove", Callisto_static_arrayFuncRemove, 0 },
	{ "array::insert", Callisto_static_arrayFuncInsert, 0 },
	{ "array::add", Callisto_static_arrayFuncAdd, 0 },
	{ "array::count", Callisto_static_arrayFuncCount, 0 },
};


//------------------------------------------------------------------------------
static const Callisto_FunctionEntry Callisto_tableIteratorFuncs[]=
{
	{ "removeCurrent", Callisto_tableIFuncRemoveCurrent, 0 },
	{ "remove", Callisto_tableFuncRemove, 0 },
	{ "insert", Callisto_tableFuncInsert, 0 },
	{ "reset", Callisto_tableIFuncReset, 0 },
};

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry Callisto_tableFuncs[]=
{
	{ "remove", Callisto_tableFuncRemove, 0 },
	{ "insert", Callisto_tableFuncInsert, 0 },
	{ "count", Callisto_tableFuncCount, 0 },
};

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry Callisto_static_tableFuncs[]=
{
	{ "table::remove", Callisto_static_tableFuncRemove, 0 },
	{ "table::insert", Callisto_static_tableFuncInsert, 0 },
	{ "table::count", Callisto_static_tableFuncCount, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importIterators( Callisto_Context *C )
{
	Callisto_setTypeMethods( C, Callisto_ArgArrayIterator, Callisto_arrayIteratorFuncs, sizeof(Callisto_arrayIteratorFuncs) / sizeof(Callisto_FunctionEntry) );
	Callisto_setTypeMethods( C, Callisto_ArgArray, Callisto_arrayFuncs, sizeof(Callisto_arrayFuncs) / sizeof(Callisto_FunctionEntry) );
	Callisto_importFunctionList( C, Callisto_static_arrayFuncs, sizeof(Callisto_static_arrayFuncs) / sizeof(Callisto_FunctionEntry) );

	Callisto_setTypeMethods( C, Callisto_ArgTableIterator, Callisto_tableIteratorFuncs, sizeof(Callisto_tableIteratorFuncs) / sizeof(Callisto_FunctionEntry) );
	Callisto_setTypeMethods( C, Callisto_ArgTable, Callisto_tableFuncs, sizeof(Callisto_tableFuncs) / sizeof(Callisto_FunctionEntry) );
	Callisto_importFunctionList( C, Callisto_static_tableFuncs, sizeof(Callisto_static_tableFuncs) / sizeof(Callisto_FunctionEntry) );
}
