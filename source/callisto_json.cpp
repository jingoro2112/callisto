#include "../include/callisto.h"
#include "vm.h"
#include "json_parser.h"

// todo..

//------------------------------------------------------------------------------
Callisto_Handle static_jsonImport( Callisto_ExecutionContext* E )
{
	const Value* var = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v : E->Args;
	JsonValue json;
	
	if ( var->type == CTYPE_STRING )
	{

	}
	else if ( var->type == CTYPE_ARRAY )
	{
		
	}
	else if ( var->type == CTYPE_TABLE )
	{
		
	}
	
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_jsonExport( Callisto_ExecutionContext* E )
{
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
const Callisto_FunctionEntry Callisto_jsonStatic[]=
{
	{ "json::import", static_jsonImport, 0 },
	{ "json::export", static_jsonExport, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importJson( Callisto_Context *C )
{
//	Callisto_setTypeMethods( C, Callisto_ArgString, Callisto_string, sizeof(Callisto_string) / sizeof(Callisto_FunctionEntry) );
//	Callisto_importFunctionList( C, Callisto_stringStatic, sizeof(Callisto_stringStatic) / sizeof(Callisto_FunctionEntry) );
}
