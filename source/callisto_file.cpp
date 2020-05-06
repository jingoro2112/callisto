#include "../include/callisto.h"
#include "vm.h"

//------------------------------------------------------------------------------
Callisto_Handle fileRead( Callisto_ExecutionContext* E )
{
	const char *name;
	if ( !Callisto_getStringArg(E, 0, &name) )
	{
		return 0;
	}

#ifdef _WIN32
	struct _stat sbuf;
	int ret = _stat( name, &sbuf );
#else
	struct stat sbuf;
	int ret = stat( name, &sbuf );
#endif

	if ( ret != 0 )
	{
		return 0;
	}

	FILE *infil = fopen( name, "rb" );
	if ( !infil )
	{
		return false;
	}
	
	Callisto_Handle H = Callisto_setValue( E, "" );
	Value* V = getValueFromHandle( E->callisto, H );
	V->string->setSize( sbuf.st_size, false );
	if ( fread(V->string->p_str(), sbuf.st_size, 1, infil) != 1 )
	{
		fclose( infil );
		Callisto_releaseValue( E->callisto, H );
		return 0;
	}

	fclose( infil );
	return H;
}

//------------------------------------------------------------------------------
Callisto_Handle fileWrite( Callisto_ExecutionContext* E )
{
	const char *name;
	if ( E->numberOfArgs < 2
		 || !Callisto_getStringArg(E, 0, &name) )
	{
		return 0;
	}

	const Value* data = (E->Args[1].type == CTYPE_REFERENCE) ? E->Args[1].v : E->Args + 1;
	if ( data->type != CTYPE_STRING )
	{
		return 0;
	}

	if ( !data->string->bufferToFile( name, false ) )
	{
		return 0;
	}

	return Callisto_setValue( E, (int64_t)data->string->size() );
}

//------------------------------------------------------------------------------
Callisto_Handle fileAppend( Callisto_ExecutionContext* E )
{
	const char *name;
	if ( E->numberOfArgs < 2
		 || !Callisto_getStringArg(E, 0, &name) )
	{
		return 0;
	}

	const Value* data = (E->Args[1].type == CTYPE_REFERENCE) ? E->Args[1].v : E->Args + 1;
	if ( data->type != CTYPE_STRING )
	{
		return 0;
	}

	if ( !data->string->bufferToFile(name, true) )
	{
		return 0;
	}

	return Callisto_setValue( E, (int64_t)data->string->size() );
}

//------------------------------------------------------------------------------
Callisto_Handle fileDelete( Callisto_ExecutionContext* E )
{
	const char *name;
	if ( Callisto_getStringArg(E, 0, &name) )
	{
#ifdef _WIN32
		_unlink( name );
#else
		unlink( name );
#endif
	}

	return 0;
}

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry fileFuncs[]=
{
	{ "file::read", fileRead, 0 },
	{ "file::write", fileWrite, 0 },
	{ "file::append", fileAppend, 0 },
	{ "file::delete", fileDelete, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importFile( Callisto_Context *C )
{
	Callisto_importFunctionList( C, fileFuncs, sizeof(fileFuncs) / sizeof(Callisto_FunctionEntry) );
}

