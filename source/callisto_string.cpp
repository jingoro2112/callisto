#include "../include/callisto.h"
#include "vm.h"

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
				Callisto_setArrayValue( E, terms, Callisto_setValue(E, token.c_str(), token.size()) );
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

		Callisto_setArrayValue( E, terms, Callisto_setValue(E, token.c_str(), token.size()) );
	}
}

//------------------------------------------------------------------------------
Callisto_Handle ltrim( Callisto_ExecutionContext* E )
{
	E->object.string->ltrim();
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_ltrim( Callisto_ExecutionContext* E )
{
	const Value* string = getValueFromArg(E, 0);

	if ( string && string->type == CTYPE_STRING )
	{
		Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
		Value* V = getValueFromHandle( E->callisto, H );
		V->string->ltrim();
		return H;
	}
	
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle rtrim( Callisto_ExecutionContext* E )
{
	E->object.string->rtrim();
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_rtrim( Callisto_ExecutionContext* E )
{
	const Value* string = getValueFromArg(E, 0);

	if ( string && string->type == CTYPE_STRING )
	{
		Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
		Value* V = getValueFromHandle( E->callisto, H );
		V->string->rtrim();
		return H;
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle trim( Callisto_ExecutionContext* E )
{
	E->object.string->trim();
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_trim( Callisto_ExecutionContext* E )
{
	const Value* string = getValueFromArg(E, 0);
	if ( string && string->type == CTYPE_STRING )
	{
		Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
		Value* V = getValueFromHandle( E->callisto, H );
		V->string->trim();
		return H;
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle split( Callisto_ExecutionContext* E )
{
	const char* string = E->object.string->c_str();

	Callisto_Handle H = Callisto_createArray( E );

	const char* data;
	unsigned int len;

	if ( Callisto_getStringArg(E, 0, &data, &len) && (len > 0) ) 
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
Callisto_Handle static_split( Callisto_ExecutionContext* E )
{
	const Value* string = getValueFromArg(E, 0);
	if ( string && string->type == CTYPE_STRING )
	{
		const char* data;
		unsigned int len;
		Callisto_Handle H = Callisto_createArray( E );
		
		if ( Callisto_getStringArg(E, 1, &data, &len) && (len > 0) )
		{
			tokenize( E, string->string->c_str(), H, data[0] );
		}
		else
		{
			tokenize( E, string->string->c_str(), H );
		}

		return H;
	}
	
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle grep( Callisto_ExecutionContext* E )
{
/*
	const char* base = Callisto_getStringArg( E, 0 );
	const char* expression = Callisto_getStringArg( E, 1 );
	if ( !base || !expression )
	{
		return 0;
	}

	regex_t regex;

	if ( regcomp( &regex, expression, 0) )
	{
		return 0;
	}
	
	int ret = regexec( &regex, base, 0, NULL, 0 );

	regfree( &regex );

	return Callisto_setValue( E, (ret == 0) ? (int64_t)1 : (int64_t)0 );
*/
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle truncate( Callisto_ExecutionContext* E )
{
	E->object.string->truncate( (unsigned int)Callisto_getIntArg(E, 0) );
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_truncate( Callisto_ExecutionContext* E )
{
	const Value* string = getValueFromArg(E, 0);
	if ( string && string->type == CTYPE_STRING )
	{
		Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
		Value* V = getValueFromHandle( E->callisto, H );
		V->string->truncate( (unsigned int)Callisto_getIntArg(E, 1) );
		return H;
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle shave( Callisto_ExecutionContext* E )
{
	E->object.string->shave( (unsigned int)Callisto_getIntArg(E, 0) );
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_shave( Callisto_ExecutionContext* E )
{
	const Value* string = getValueFromArg(E, 0);
	if ( string && string->type == CTYPE_STRING )
	{
		Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
		Value* V = getValueFromHandle( E->callisto, H );
		V->string->shave( (unsigned int)Callisto_getIntArg(E, 1) );
		return H;
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle shift( Callisto_ExecutionContext* E ) 
{
	E->object.string->shift( (unsigned int)Callisto_getIntArg(E, 0),
							 (unsigned int)Callisto_getIntArg(E, 1) );
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_shift( Callisto_ExecutionContext* E )
{
	const Value* string = getValueFromArg(E, 0);
	if ( string && string->type == CTYPE_STRING )
	{
		Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
		Value* V = getValueFromHandle( E->callisto, H );
		V->string->shift( (unsigned int)Callisto_getIntArg(E, 1),
								(unsigned int)Callisto_getIntArg(E, 2) );
		return H;
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle replace( Callisto_ExecutionContext* E ) 
{
	const char* s1 = Callisto_getStringArg( E, 0 );
	const char* s2 = Callisto_getStringArg( E, 1 );
	if ( s1 && s2 )
	{
		E->object.string->replace( s1, s2 );
	}

	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_replace( Callisto_ExecutionContext* E )
{
	const Value* string = getValueFromArg(E, 0);
	if ( string && string->type == CTYPE_STRING )
	{
		const char* s1 = Callisto_getStringArg( E, 1 );
		const char* s2 = Callisto_getStringArg( E, 2 );
		if ( s1 && s2 )
		{
			Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
			Value* V = getValueFromHandle( E->callisto, H );
			V->string->replace( s1, s2 );
			return H;
		}
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle length( Callisto_ExecutionContext* E ) 
{
	return Callisto_setValue( E, (int64_t)(E->object.string->size()) );
}

//------------------------------------------------------------------------------
Callisto_Handle static_length( Callisto_ExecutionContext* E ) 
{
	const Value* string = getValueFromArg( E, 0 );
	if ( string && string->type == CTYPE_STRING )
	{
		return Callisto_setValue( E, (int64_t)(string->string->size()) );
	}
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle find( Callisto_ExecutionContext* E ) 
{
	bool matchCase = true;
	if ( E->numberOfArgs > 1 )
	{
		matchCase = getValueFromArg(E, 1)->i64 != 0;
	}
	
	if ( E->numberOfArgs > 0 )
	{
		const Value *V2 = getValueFromArg(E, 0);
		if ( V2->type == CTYPE_STRING )
		{
			const char* found = E->object.string->find( *V2->string, matchCase );
			if ( found )
			{
				return Callisto_setValue( E, (int64_t)(found - E->object.string->c_str()) );
			}
		}
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_find( Callisto_ExecutionContext* E ) 
{
	const Value* V1 = getValueFromArg( E, 0 );
	if ( V1 && V1->type == CTYPE_STRING )
	{
		bool matchCase = true;
		if ( E->numberOfArgs > 2 )
		{
			matchCase = getValueFromArg( E, 2 )->i64 != 0;
		}

		if ( E->numberOfArgs > 1 )
		{
			const Value *V2 = getValueFromArg( E, 1 );
			if ( V2 && V2->type == CTYPE_STRING )
			{
				const char* found = V1->string->find( *V2->string, matchCase );
				if ( found )
				{
					return Callisto_setValue( E, (int64_t)(found - V1->string->c_str()) );
				}
			}
		}
	}
	
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle lower( Callisto_ExecutionContext* E ) 
{
	E->object.string->toLower();
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_lower( Callisto_ExecutionContext* E ) 
{
	const Value* string = getValueFromArg( E, 0 );
	if ( string && string->type == CTYPE_STRING )
	{
		Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
		Value* V = getValueFromHandle( E->callisto, H );
		V->string->toLower();
		return H;
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle upper( Callisto_ExecutionContext* E ) 
{
	E->object.string->toUpper();
	return Callisto_THIS_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_upper( Callisto_ExecutionContext* E ) 
{
	const Value* string = getValueFromArg( E, 0 );
	if ( string && string->type == CTYPE_STRING )
	{
		Callisto_Handle H = Callisto_setValue( E, string->string->c_str(), string->string->size() );
		Value* V = getValueFromHandle( E->callisto, H );
		V->string->toUpper();
		return H;
	}

	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
inline Callisto_Handle substringEx( Callisto_ExecutionContext* E, Cstr const& item, unsigned int from, unsigned int to )
{
	return (from >= item.size() || to > item.size() || to < from) ? Callisto_setValue( E, item.subStr(from, to) )
																  : Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle substring( Callisto_ExecutionContext* E ) 
{
	Cstr const& item = *E->object.string;
	unsigned int from = (unsigned int)Callisto_getIntArg(E, 0);
	unsigned int to = (unsigned int)Callisto_getIntArg(E, 1);
	if ( from < item.size()
		 && to <= item.size()
		 && to >= from )
	{
		return Callisto_setValue( E, item.subStr(from, to), to - from );
	}
	
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
Callisto_Handle static_substring( Callisto_ExecutionContext* E ) 
{
	const Value* string = getValueFromArg( E, 0 );
	if ( string && string->type == CTYPE_STRING )
	{
		Cstr const& item = *string->string;
		unsigned int from = (unsigned int)Callisto_getIntArg(E, 1);
		unsigned int to = (unsigned int)Callisto_getIntArg(E, 2);
		if ( from < item.size()
			 && to <= item.size()
			 && to >= from )
		{
			return Callisto_setValue( E, item.subStr(from, to), to - from );
		}
	}
	
	return Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
inline Callisto_Handle toArrayEx( Callisto_ExecutionContext* E, const char* string )
{
	Callisto_Handle H = Callisto_createArray( E );
	for( unsigned int pos=0; string[pos]; ++pos )
	{
		Callisto_setArrayValue( E, H, Callisto_setValue(E, string + pos, 1) );
	}
	return H;
}

//------------------------------------------------------------------------------
Callisto_Handle toArray( Callisto_ExecutionContext* E ) 
{
	return toArrayEx( E, E->object.string->c_str() );
}

//------------------------------------------------------------------------------
Callisto_Handle static_toArray( Callisto_ExecutionContext* E ) 
{
	const Value* string = getValueFromArg(E, 0);
	return (string && string->type == CTYPE_STRING) ? toArrayEx( E, string->string->c_str() ) : Callisto_NULL_HANDLE;
}

//------------------------------------------------------------------------------
const Callisto_FunctionEntry Callisto_string[]=
{
	{ "ltrim", ltrim, 0 },
	{ "rtrim", rtrim, 0 },
	{ "trim", trim, 0 },
	{ "split", split, 0 },
	{ "grep", grep, 0 },
	{ "truncate", truncate, 0 },
	{ "shave", shave, 0 },
	{ "shift", shift, 0 },
	{ "replace", replace, 0 },
	{ "length", length, 0 },
	{ "find", find, 0 },
	{ "lower", lower, 0 },
	{ "upper", upper, 0 },
	{ "substring", substring, 0 },
	{ "toarray", toArray, 0 },
};

//------------------------------------------------------------------------------
const Callisto_FunctionEntry Callisto_stringStatic[]=
{
	{ "string::ltrim", static_ltrim, 0 },
	{ "string::rtrim", static_rtrim, 0 },
	{ "string::trim", static_trim, 0 },
	{ "string::split", static_split, 0 },
	{ "string::truncate", static_truncate, 0 },
	{ "string::shave", static_shave, 0 },
	{ "string::shift", static_shift, 0 },
	{ "string::replace", static_replace, 0 },
	{ "string::length", static_length, 0 },
	{ "string::find", static_find, 0 },
	{ "string::lower", static_lower, 0 },
	{ "string::upper", static_upper, 0 },
	{ "string::substring", static_substring, 0 },
	{ "string::toarray", static_toArray, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importString( Callisto_Context *C )
{
	Callisto_setTypeMethods( C, Callisto_ArgString, Callisto_string, sizeof(Callisto_string) / sizeof(Callisto_FunctionEntry) );
	Callisto_importFunctionList( C, Callisto_stringStatic, sizeof(Callisto_stringStatic) / sizeof(Callisto_FunctionEntry) );
}
