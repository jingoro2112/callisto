#include "include/callisto.h"

#include <stdio.h>

#include "source/util.h"
#include "source/str.hpp"
#include "source/asciidump.hpp"
#include "source/log.hpp"
CLog Log;

//------------------------------------------------------------------------------
int usage()
{
	printf( "Usage: callisto <command> <source> [<target>]\n"
			"where command is:\n"
			"c       compile <source>\n"
			"r       run <compiled code>\n"
		  );
	return -1;
}

//------------------------------------------------------------------------------
void TMCallback( tm& timeStruct )
{
	time_t tod = time(0);
	memcpy( &timeStruct, localtime(&tod), sizeof(struct tm) );
}

//------------------------------------------------------------------------------
int main( int argn, char** argv )
{
	Log.setTMCallback( TMCallback );

	if ( argn < 2 )
	{
		return usage();
	}

	if ( *argv[1] == 'c' )
	{
		Cstr source;
		if ( !source.fileToBuffer( argv[2] ) )
		{
			printf( "Could not open for compile [%s]\n", argv[2] );
			return usage();
		}

		char* out;
		unsigned int outLen;
		Callisto_parse( source.c_str(), source.size(), &out, &outLen );

		Cstr bufOut;
		bufOut.set( out, outLen );
		if ( argn > 3 )
		{
			bufOut.bufferToFile( argv[3] );
		}
		else
		{
			asciiDump( out, outLen );
		}
		
		delete[] out;
	}
	else if ( *argv[1] == 'r' )
	{
		Callisto_Context C;
		Callisto_importStdlib( &C );
		Callisto_ExecutionContext* E = Callisto_createExecutionContext( &C );

		Callisto_Handle H = Callisto_createValue( E );

		for( int a = 3; a<argn; ++a )
		{
			Callisto_setArrayValue( E, H, a - 3, Callisto_createValue(E, argv[a], (int)strlen(argv[a])) );
		}

		if ( Callisto_loadFile(E, argv[2], H) )
		{
			printf( "loadFile err[%d]: [%s]\n",  Callisto_lastErr(), Callisto_formatError(Callisto_lastErr()) );
			return -1;
		}

		Callisto_releaseValue( E, H );
	}
	else if ( *argv[1] == 't' )
	{
		Callisto_tests( (argn >= 3) ? atoi(argv[2]) : 0 );
	}
	else
	{
		return usage();
	}
		
	return 0;
}
