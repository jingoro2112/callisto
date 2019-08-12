#include "source/util.h"

#include <assert.h>

Cstr log;
   
//------------------------------------------------------------------------------
Callisto_Handle emit( Callisto_ExecutionContext* F )
{
	for( int i=0; i<F->numberOfArgs; i++ )
	{
		log.appendFormat( "%s", formatValue(F->Args[i]).c_str() );
	}
	
	log += "\n";
	return 0;
}

//------------------------------------------------------------------------------
Callisto_Handle typeof( Callisto_ExecutionContext* F )
{
	switch( Callisto_getArgType(F, 0) )
	{
		case CTYPE_NULL: return Callisto_createValue( F, "null" ); break;
		case CTYPE_INT: return Callisto_createValue( F, "integer" ); break;
		case CTYPE_FLOAT: return Callisto_createValue( F, "float" ); break;
		case CTYPE_STRING: return Callisto_createValue( F, "string" ); break;
		case CTYPE_WSTRING: return Callisto_createValue( F, "wstring" ); break;
		case CTYPE_UNIT: return Callisto_createValue( F, "unit" ); break;
		case CTYPE_TABLE: return Callisto_createValue( F, "table" ); break;
		case CTYPE_ARRAY: return Callisto_createValue( F, "array" ); break;
	}

	return 0;
}

//------------------------------------------------------------------------------
const Callisto_FunctionEntry logFuncs[]=
{
	{ "log", emit, 0 },
	{ "typeof", typeof, 0 },
};

//------------------------------------------------------------------------------
int Callisto_tests( int runTest )
{
	Cstr code;
	Cstr codeName;

	int index = 1;
	if ( runTest != 0 )
	{
		index = runTest;
	}

	Cstr expect;

	for(;;++index)
	{
		codeName.format( "tests/test_%d.c", index );
		if ( !code.fileToBuffer(codeName) )
		{
			break;
		}

		log.clear();
		expect.clear();

		unsigned int t = 0;
		
		for( ; (t < code.size()) && (code[t] != '~'); ++t );
		
		t += 2;
		
		for( ; t<code.size() && (code[t] != '~'); ++t )
		{
			expect += code[t];
		}

		//printf( "Code expecting [%s]\n", expect.c_str() );

		printf( "test [%d]: ", index );
			
		Callisto_Context C;
		Callisto_importList( &C, logFuncs, sizeof(logFuncs) / sizeof(Callisto_FunctionEntry) );

		Callisto_importStdlib( &C );
		Callisto_importString( &C );

		Callisto_ExecutionContext* E = Callisto_createExecutionContext( &C );

		int err = Callisto_load( E, code.c_str(), code.size() );
		if ( err )
		{
			printf( "execute error[%d] with [%s]\n", err, codeName.c_str() );
			return index;
		}

		if ( log != expect )
		{
			printf( "error: expected\n"
					"-----------------------------\n"
					"%s\n"
					"saw:-------------------------\n"
					"%s"
					"\n"
					"-----------------------------\n",
					expect.c_str(), log.c_str() );
			return index;
		}

		printf( "PASS\n" );
		
		if ( runTest )
		{
			break;
		}
	}

	return 0;
}