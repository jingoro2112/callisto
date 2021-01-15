#include "source/c_str.h"
#include "source/callisto.h"
#include "source/vm.h"

#include <assert.h>

C_str logstr;
   
//------------------------------------------------------------------------------
static Callisto_Handle emit( Callisto_ExecutionContext* E )
{
	for( int i=0; i<E->numberOfArgs; i++ )
	{
		logstr.appendFormat( "%s", formatValue(E->Args[i]).c_str() );
	}
	
	logstr += "\n";
	return 0;
}

//------------------------------------------------------------------------------
static Callisto_Handle c_wait( Callisto_ExecutionContext* E )
{
	for( int i=0; i<E->numberOfArgs; i++ )
	{
		logstr.appendFormat( "%s\n", formatValue(E->Args[i]).c_str() );
	}

	Callisto_sleep( 1000 );
	
	return 0;
}

//------------------------------------------------------------------------------
const Callisto_FunctionEntry logFuncs[]=
{
	{ "log", emit, 0 },

	{ "c_wait", c_wait, 0 },
};

//------------------------------------------------------------------------------
void trace( const char* code, const unsigned int line, const unsigned int col, const int thread )
{
	printf( "%s", code );
}

//------------------------------------------------------------------------------
int Callisto_tests( int runTest, bool debug )
{
	C_str code;
	C_str codeName;

	int err = 0;

	assert( O_LAST <= 254 ); // opcodes MUST fit into 8 bits, don't accidentally blow this :P

	FILE* tfile = fopen( "test_files.txt", "r" );
	char buf[256];
	int fileNumber = 0;
	while( fgets(buf, 255, tfile) )
	{
		if ( !runTest || (runTest == fileNumber) )
		{
			C_str expect;

			codeName = buf;
			codeName.trim();
			if ( code.fileToBuffer(codeName) )
			{
				unsigned int t = 0;

				for( ; (t < code.size()) && (code[t] != '~'); ++t );

				t += 2;

				for( ; t<code.size() && (code[t] != '~'); ++t )
				{
					expect += code[t];
				}

				//printf( "Code expecting [%s]\n", expect.c_str() );

				printf( "test [%d][%s]: ", fileNumber, codeName.c_str() );

//				for(;;)
//				for( int i=0; i<10; ++i )
				{
					Callisto_RunOptions options;
					options.traceCallback = debug ? trace : 0;
					Callisto_Context C( &options );

					Callisto_importFunctionList( &C, logFuncs, sizeof(logFuncs) / sizeof(Callisto_FunctionEntry) );
					Callisto_importAll( &C );

//					for( int j=0;j<10;++j )
					{
						logstr.clear();
						err = Callisto_run( &C, code.c_str(), code.size() );
						if ( err )
						{
							printf( "execute error[%d] with [%s]\n", err, codeName.c_str() );
							break;
						}
					}
				}

				if ( logstr != expect )
				{
					printf( "error: expected\n"
							"-----------------------------\n"
							"%s\n"
							"saw:-------------------------\n"
							"%s"
							"\n"
							"-----------------------------\n",
							expect.c_str(), logstr.c_str() );
					
					break;
				}

				printf( "PASS\n" );

				if ( runTest )
				{
					break;
				}
			}
			else if ( fileNumber != 0 )
			{
				printf( "test [%d][%s]: FILE NOT FOUND\n", fileNumber, codeName.c_str() );
			}
		}

		fileNumber++;
	}

	fclose( tfile );
	return err;
}
