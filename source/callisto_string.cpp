#include "util.h"

//------------------------------------------------------------------------------
void tokenize( Callisto_ExecutionContext* E, const char* str, Callisto_Handle terms, const char delimeter =0 )
{
	// for delimeter == '0' whitespace is assumed
	unsigned int pos = 0;
	Cstr token;

	for(;;)
	{
		token.clear();

		if ( !str[pos] )
		{
			if ( str[pos - 1] == delimeter )
			{
				Callisto_addArrayValue( E, terms, Callisto_createValue(E, token.c_str(), token.size()) );
			}

			break;
		}

		if ( delimeter )
		{
			while ( str[pos] && (str[pos] != delimeter) )
			{
				token += str[pos++];
			} 

			if ( str[pos] )
			{
				pos++;  // skip delimeter
			}
		}
		else
		{
			if (str[pos] == '\"')
			{
				pos++; // skip leading quote
				while (str[pos] && str[pos] != '\"')
				{
					token += str[pos++];
				}

				if ( str[pos] )
				{
					pos++;  // skip delimeter
				}
			}
			else
			{
				while( str[pos] && !isspace(str[pos]) )
				{
					token += str[pos++];
				}
			}

			for( ;str[pos] && isspace(str[pos]); pos++ ); // soak up whitespace
		}

		Callisto_addArrayValue( E, terms, Callisto_createValue(E, token.c_str(), token.size()) );
	}
}


//------------------------------------------------------------------------------
Callisto_Handle ltrim( Callisto_ExecutionContext* E )
{
	return 0;
}

//------------------------------------------------------------------------------
Callisto_Handle rtrim( Callisto_ExecutionContext* E )
{
	return 0;
}

//------------------------------------------------------------------------------
Callisto_Handle trim( Callisto_ExecutionContext* E )
{
	const Value *V = (E->Args[0].type == CTYPE_OFFSTACK_REFERENCE) ? E->Args[0].v : E->Args;
	if ( V->type == CTYPE_STRING )
	{
		V->refCstr->item.trim();
	}

	return 0;
}

//------------------------------------------------------------------------------
Callisto_Handle split( Callisto_ExecutionContext* E ) //delim, inp)  like perl split, take a string and split based on delimeter into pieces, return an array of the pieces
{
	const char* string = Callisto_getStringArg( E, 1 );
	if ( !string )
	{
		return 0;
	}

	Callisto_Handle H = Callisto_createValue( E );

	char data[2];
	unsigned int len = 2;

	if ( Callisto_getStringArg(E, 2, data, &len) )
	{
		tokenize( E, string, H, data[0] );
	}
	else
	{
		tokenize( E, string, H );
	}

	return H;
}

//------------------------------------------------------------------------------
Callisto_Handle grep( Callisto_ExecutionContext* E ) // -- trivial grep that returns lines that match a pattern, ignore case, et
{
	return 0;
}

//------------------------------------------------------------------------------
const Callisto_FunctionEntry Callisto_string[]=
{
	{ "ltrim", ltrim, 0 },
	{ "rtrim", rtrim, 0 },
	{ "trim", trim, 0 },
	{ "split", split, 0 },
	{ "grep", grep, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importString( Callisto_Context *C )
{
	Callisto_setGlobalMetaTable( C, CTYPE_STRING, Callisto_string, sizeof(Callisto_string) / sizeof(Callisto_FunctionEntry) );
}

