#include "../include/callisto.h"
#include "vm.h"

#ifndef _WIN32
#include <unistd.h>
#endif

#include <stdio.h>
#include "random.h"

CRandom callisto_rand;

//------------------------------------------------------------------------------
static Callisto_Handle printf( Callisto_ExecutionContext* E )
{
	if ( !E->numberOfArgs )
	{
		return 0;
	}

	char format[4000];
	unsigned int len = 3999;
	
	Callisto_formatArg( E, 0, format, &len );
	
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
						str.appendFormat( formatString, (int)Callisto_getIntArg(E, onarg) );
						terminatorFound = true;
						break;
					}

					case 's':
					{
						char buf[128];
						unsigned int len = 127;
						str.appendFormat( formatString, Callisto_formatArg(E, onarg, buf, &len) );
						terminatorFound = true;
						break;
					}

					case 'f':
					case 'g':
					{
						str.appendFormat( formatString, Callisto_getFloatArg(E, onarg) );
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
static Callisto_Handle printl( Callisto_ExecutionContext* E )
{
	for( int i=0; i<E->numberOfArgs; i++ )
	{
		printf( "%s", formatValue(E->Args[i]).c_str() );
	}

	printf( "\n" );
	return 0;
}

//------------------------------------------------------------------------------
static Callisto_Handle ranint( Callisto_ExecutionContext* E )
{
	int from = (int)Callisto_getIntArg( E, 0 );
	int to = (int)Callisto_getIntArg( E, 1 );
	return Callisto_setValue( E, (int64_t)callisto_rand.fromToInclusive(from, to) );
}

//------------------------------------------------------------------------------
static Callisto_Handle sranint( Callisto_ExecutionContext* E )
{
	callisto_rand.seed ( (int)Callisto_getIntArg( E, 0 ) );
	return 0;
}

//------------------------------------------------------------------------------
static Callisto_Handle epochtime( Callisto_ExecutionContext* E )
{
	return Callisto_setValue( E, (int64_t)time(0) );
}

//------------------------------------------------------------------------------
#ifdef __linux__
#include <sys/time.h>
static Callisto_Handle dogetline( Callisto_ExecutionContext* E )
{
	char* buffer = 0;
	size_t bufsize = 0;
	Callisto_Handle ret = 0;
	if ( getline( &buffer, &bufsize, stdin ) != -1 )
	{
		ret = Callisto_setValue( E, buffer );
		free( buffer );
	}

	return ret;
}

//------------------------------------------------------------------------------
static Callisto_Handle getTimeOfDay( Callisto_ExecutionContext* E )
{
	struct timeval tv;
	gettimeofday( &tv, 0 );
	return Callisto_setValue( E, (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.) );
}

int gettimeofday(struct timeval *tv, struct timezone *tz);

#endif

#ifdef _WIN32
#include <iostream>
#include <string>
using namespace std;
static Callisto_Handle dogetline( Callisto_ExecutionContext* E )
{
	string line;
	getline(cin, line);
	return Callisto_setValue( E, line.c_str() );
}

//------------------------------------------------------------------------------
static Callisto_Handle getTimeOfDay( Callisto_ExecutionContext* E )
{
	FILETIME ft;
	GetSystemTimeAsFileTime( &ft );
	ULARGE_INTEGER ftui;
	ftui.LowPart = ft.dwLowDateTime;
	ftui.HighPart = ft.dwHighDateTime;
	return Callisto_setValue( E, (double)time(0) + (double)((long long)ftui.QuadPart % 10000000) / 10000000. );
}

#endif

//------------------------------------------------------------------------------
static Callisto_Handle type_of( Callisto_ExecutionContext* E )
{
	int type = -1;
	if ( E->numberOfArgs > 0 )
	{
		type = (E->Args->type == CTYPE_REFERENCE) ? E->Args->v->type : E->Args->type;
	}

	switch( type )
	{
		case CTYPE_NULL: return Callisto_setValue( E, "null" ); break;
		case CTYPE_INT: return Callisto_setValue( E, "integer" ); break;
		case CTYPE_FLOAT: return Callisto_setValue( E, "float" ); break;
		case CTYPE_STRING: return Callisto_setValue( E, "string" ); break;
		case CTYPE_UNIT: return Callisto_setValue( E, "unit" ); break;
		case CTYPE_TABLE: return Callisto_setValue( E, "table" ); break;
		case CTYPE_ARRAY: return Callisto_setValue( E, "array" ); break;
		default: return 0;
	}
}

//------------------------------------------------------------------------------
Callisto_Handle string_count( Callisto_ExecutionContext* E )
{
	return Callisto_setValue( E, (int64_t)E->object.string->size() );
}

//------------------------------------------------------------------------------
Callisto_Handle table_count( Callisto_ExecutionContext* E )
{
	return Callisto_setValue( E, (int64_t)E->object.tableSpace->count() );
}

//------------------------------------------------------------------------------
Callisto_Handle array_count( Callisto_ExecutionContext* E )
{
	return Callisto_setValue( E, (int64_t)E->object.array->count() );
}

//------------------------------------------------------------------------------
const static Callisto_FunctionEntry stringFunctions[]=
{
	{ "count", string_count, 0 },
};

//------------------------------------------------------------------------------
const static Callisto_FunctionEntry tableFunctions[]=
{
	{ "count", table_count, 0 },
};

//------------------------------------------------------------------------------
const static Callisto_FunctionEntry arrayFunctions[]=
{
	{ "count", array_count, 0 },
};

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry Callisto_stdlib[]=
{
	{ "print", printf, 0 },
	{ "printf", printf, 0 },
	{ "printl", printl, 0 },
	{ "stlib::print", printf, 0 },
	{ "stlib::printf", printf, 0 },
	{ "stlib::printl", printl, 0 },
	{ "stlib::getline", dogetline, 0 },
	{ "stlib::random", ranint, 0 },
	{ "stlib::srandom", sranint, 0 },
	{ "stlib::epoch", epochtime, 0 },
	{ "stlib::time", getTimeOfDay, 0 },
	{ "stlib::typeof", type_of, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importStdlib( Callisto_Context *C )
{
	Callisto_importFunctionList( C, Callisto_stdlib, sizeof(Callisto_stdlib) / sizeof(Callisto_FunctionEntry) );

	Callisto_setTypeMethods( C, Callisto_ArgStringIterator, stringFunctions, sizeof(stringFunctions) / sizeof(Callisto_FunctionEntry) );
	Callisto_setTypeMethods( C, Callisto_ArgTableIterator, tableFunctions, sizeof(tableFunctions) / sizeof(Callisto_FunctionEntry) );
	Callisto_setTypeMethods( C, Callisto_ArgArrayIterator, arrayFunctions, sizeof(arrayFunctions) / sizeof(Callisto_FunctionEntry) );
}
