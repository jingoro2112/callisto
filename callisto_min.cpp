#include "include/callisto.h"

#include <stdio.h>

//------------------------------------------------------------------------------
int usage()
{
	printf( "Usage: min <bytefile>\n"
		  );
	return -1;
}

//------------------------------------------------------------------------------
int main( int argn, char** argv )
{
	if ( argn < 2 )
	{
		return usage();
	}

	Callisto_Context* C = Callisto_createContext();
	Callisto_importAll( C );

	if ( Callisto_runFileCompiled(C, argv[1]) )
	{
		printf( "ERR: [%d]\n", Callisto_lastErr() );
	}
		
	return 0;
}

//------------------------------------------------------------------------------
int Callisto_parse( const char* data, const int size, char** out, unsigned int* outLen, const bool addDebugInfo )
{
	return CE_ImportLoadError;
}