#include "util.h"

#ifndef _WIN32
#include <unistd.h>
#endif

#include <stdio.h>
#include "random.h"

CRandom callisto_rand;

//------------------------------------------------------------------------------
Callisto_Handle printf( Callisto_ExecutionContext* E )
{
	if ( !E->numberOfArgs )
	{
		return 0;
	}

	char format[4000];
	unsigned int len = 3999;
	
	Callisto_getStringArg( E, 0, format, &len );
	
	Cstr str;
	Cstr formatString;

	int onarg = 0;

	for( int i=0; format[i]; i++ )
	{
		if ( format[i] == '%' )
		{
			if ( !format[i+1] )
			{
				return 0;
			}

			i++;
			if ( format[i] == '%' )
			{
				str.append( '%' );
				continue;
			}

			if ( onarg == E->numberOfArgs )
			{
				return 0;
			}

			onarg++;
			
			formatString = '%';

			bool terminatorFound = false;
			for( ;format[i] ; i++ )
			{
				formatString += format[i];

				switch( format[i] )
				{
					case 'd':
					case 'u':
					case 'x':
					case 'X':
					{
						if ( Callisto_getArgType(E, onarg) == CTYPE_INT )
						{
							str.appendFormat( formatString, (int)Callisto_getIntArg(E, onarg) );
						}
						else
						{
							str += "<bad %d/u/x/X>";
						}
						terminatorFound = true;
						break;
					}

					case 's':
					{
						if ( Callisto_getArgType(E, onarg) == CTYPE_STRING )
						{
							str.appendFormat( formatString, Callisto_getStringArg(E, onarg) );
						}
						else
						{
							str += "<bad %s>";
						}

						terminatorFound = true;
						break;
					}

					case 'f':
					case 'g':
					{
						if ( Callisto_getArgType(E, onarg) == CTYPE_FLOAT )
						{
							str.appendFormat( formatString, Callisto_getFloatArg(E, onarg) );
						}
						else
						{
							str += "<bad %g/%f>";
						}

						terminatorFound = true;
						break;
					}

					default:
					{
						break;
					}
				}

				if ( terminatorFound )
				{
					break;
				}
			}

			if ( !format[i] )
			{
				break;
			}
		}
		else
		{
			str.append( format[i] );
		}
	}

	printf( "%s", str.c_str() );
	return 0;
}

//------------------------------------------------------------------------------
Callisto_Handle printl( Callisto_ExecutionContext* E )
{
	for( int i=0; i<E->numberOfArgs; i++ )
	{
		printf( "%s", formatValue(E->Args[i]).c_str() );
	}

	printf( "\n" );
	return 0;
}

//------------------------------------------------------------------------------
Callisto_Handle ranint( Callisto_ExecutionContext* E )
{
	int from = Callisto_getIntArg( E, 0 );
	int to = Callisto_getIntArg( E, 1 );
	return Callisto_createValue( E, (int64_t)callisto_rand.fromToInclusive(from, to) );
}

//------------------------------------------------------------------------------
#ifdef __linux__
Callisto_Handle dogetline( Callisto_ExecutionContext* E )
{
	char* buffer = 0;
	size_t bufsize = 0;
	Callisto_Handle ret = 0;
	if ( getline( &buffer, &bufsize, stdin ) != -1 )
	{
		ret = Callisto_createValue( E, buffer );
		free( buffer );
	}

	return ret;
}
#endif

#ifdef _WIN32
#include <iostream>
#include <string>
using namespace std;
Callisto_Handle dogetline( Callisto_ExecutionContext* E )
{
	string line;
	getline(cin, line);
	return Callisto_createValue( E, line.c_str() );
}
#endif

//------------------------------------------------------------------------------
const Callisto_FunctionEntry Callisto_stdlib[]=
{
	{ "print", printf, 0 },
	{ "printf", printf, 0 },
	{ "printl", printl, 0 },
	{ "getline", dogetline, 0 },
	{ "random", ranint, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importStdlib( Callisto_Context *C )
{
	Callisto_importList( C, Callisto_stdlib, sizeof(Callisto_stdlib) / sizeof(Callisto_FunctionEntry) );
}

