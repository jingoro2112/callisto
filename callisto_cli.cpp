#include "source/callisto.h"

#include <stdio.h>

#include "source/c_str.h"
#include "source/asciidump.h"
#include "source/c_log.h"

extern Callisto::CLog C_Log;

using namespace Callisto;

int Callisto_tests( int runTest =0, bool debug =false );

//------------------------------------------------------------------------------
int usage()
{
	printf( "Usage: callisto <command> <source> [<target>]\n"
			"where command is:\n"
			"c       compile <source>\n"
			"d       compile with debug <source>\n"
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
	C_Log.setTMCallback( TMCallback );

	if ( argn < 2 )
	{
		return usage();
	}

	Callisto_RunOptions options;
	
	if ( *argv[1] == 'c' || *argv[1] == 'd' )
	{
		C_str source;
		if ( !source.fileToBuffer( argv[2] ) )
		{
			printf( "Could not open for compile [%s]\n", argv[2] );
			return usage();
		}

		char* out;
		unsigned int outLen;

		Callisto_parse( source.c_str(), source.size(), &out, &outLen, *argv[1] == 'd' );

		C_str bufOut;
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
		Callisto_Context* C = Callisto_createContext( &options );
		Callisto_importAll( C );
	
		if ( Callisto_runFile(C, argv[2]) )
		{
			printf( "loadFile err[%d]: [%s]\n",  Callisto_lastErr(), Callisto_formatError(Callisto_lastErr()) );
			return -1;
		}

/*
		
		options.argumentValueHandle = Callisto_createValue( C );

		for( int a = 3; a<argn; ++a )
		{
			Callisto_setArrayValue( C, options.argumentValueHandle, a - 3, Callisto_createValue(C, argv[a], (int)strlen(argv[a])) );
		}

		if ( Callisto_runFile(C, argv[2], &options) )
		{
			printf( "loadFile err[%d]: [%s]\n",  Callisto_lastErr(), Callisto_formatError(Callisto_lastErr()) );
			return -1;
		}

		Callisto_releaseValue( C, options.argumentValueHandle );
*/

		Callisto_destroyContext( C );
	}
	else if ( *argv[1] == 't' )
	{
		Callisto_tests( (argn >= 3) ? atoi(argv[2]) : 0 );
	}
	else if ( *argv[1] == 'T' )
	{
		Callisto_tests( (argn >= 3) ? atoi(argv[2]) : 0, true );
	}
	else
	{
		return usage();
	}
		
	return 0;
}
