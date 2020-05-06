#include "cc.h"
#include "utf8.h"

extern CLog Log;

#define D_TOKEN(a) //a

//------------------------------------------------------------------------------
inline bool c_isspace( const wchar_t c )
{
	return c < 127 && isspace((char)c); 
}

//------------------------------------------------------------------------------
bool parseAsString( Compilation& comp, Cstr& tokenReturn )
{
	Wstr reader;
	Wstr token;
	
	while( comp.pos < comp.source.size() )
	{
		reader.clear();
		comp.pos += UTF8ToUnicode( (unsigned char *)(comp.source.c_str() + comp.pos), reader, 1 );
		wchar_t c = reader[0];
		
		if ( c == '\"' )
		{
			// peek ahead
			bool append = false;
			for( unsigned int peek = comp.pos; peek < comp.source.size(); ++peek )
			{
				if ( c_isspace(comp.source[peek]) )
				{
					continue;
				}

				if ( comp.source[peek] == '\"' )
				{
					append = true;
					comp.pos = peek + 1;
				}

				break;
			}

			if ( !append )
			{
				unicodeToUTF8( token, token.size(), tokenReturn );
				return true;
			}
		}
		else if ( c == '\n' )
		{
			Log("Newline in string constant" );
			return false;
		}
		else if ( c == '\\' )
		{
			if ( comp.pos + 1 >= comp.source.size() )
			{
				Log("bad '\\' in string constant" );
				return false;
			}

			c = comp.source[comp.pos++];
			if ( c == '\\' ) // escaped slash
			{
				token += '\\';
			}
			else if ( c == '0' )
			{
				token += atoi( comp.source.c_str(comp.pos - 1) );
				for( ; (comp.pos < comp.source.size()) && isdigit(*comp.source.c_str(comp.pos)); ++comp.pos );
			}
			else if ( c == '\"' )
			{
				token += '\"';
			}
			else if ( c == 'n' )
			{
				token += '\n';
			}
			else if ( c == 'r' )
			{
				token += '\r';
			}
			else if ( c == 't' )
			{
				token += '\t';
			}
			else
			{
				Log("unrecognized escaped char '%c'", c );
				return false;
			}
		}
		else
		{
			token += c;
		}
	}

	Log( "unterminated string literal" );

	return false;
}

//------------------------------------------------------------------------------
bool getToken( Compilation& comp, Cstr& token, CToken& value )
{
	comp.lastPos = comp.pos;

	value.string.clear();
	value.i64 = 0;
	token.clear();
	value.type = CTYPE_NULL;
	value.spaceBefore = (comp.pos < comp.source.size()) && isspace(comp.source[comp.pos]);
	
	do
	{
		if ( comp.pos >= comp.source.size() )
		{
			return false;
		}

	} while( isspace(comp.source[comp.pos++]) );

	token = comp.source[comp.pos - 1];

	if ( token[0] == '=' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '=='"));
			token += '=';
			++comp.pos;
		}
		else
		{
			D_TOKEN(Log("token '='"));
		}
	}
	else if ( token[0] == '!' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '!='"));
			token += '=';
			++comp.pos;
		}
		else
		{
			D_TOKEN(Log("token '!'"));
		}
	}
	else if ( token[0] == ':' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == ':' )
		{
			D_TOKEN(Log("token '::'"));
			token += ':';
			++comp.pos;
		}
		else
		{
			D_TOKEN(Log("token ':'"));
		}
	}
	else if ( token[0] == '*' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '*='"));
			token += '=';
			++comp.pos;
		}
		else
		{
			D_TOKEN(Log("token '*'"));
		}
	}
	else if ( token[0] == '%' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '%='"));
			token += '=';
			++comp.pos;
		}
		else
		{
			D_TOKEN(Log("token '%'"));
		}
	}
	else if ( token[0] == '<' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '<='"));
			token += '=';
			++comp.pos;
		}
		else if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '<' )
		{
			D_TOKEN(Log("token '<<'"));
			token += '<';
			++comp.pos;

			if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
			{
				D_TOKEN(Log("token '<<='"));
				token += '=';
				++comp.pos;
			}
		}
	}
	else if ( token[0] == '>' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '>='"));
			token += '=';
			++comp.pos;
		}
		else if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '>' )
		{
			D_TOKEN(Log("token '>>'"));
			token += '>';
			++comp.pos;

			if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
			{
				D_TOKEN(Log("token '>>='"));
				token += '=';
				++comp.pos;
			}
		}
	}
	else if ( token[0] == '&' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '&' )
		{
			D_TOKEN(Log("token '&&'"));
			token += '&';
			++comp.pos;
		}
		else if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '&='"));
			token += '=';
			++comp.pos;
		}
		else
		{
			D_TOKEN(Log("token '&'"));
		}
	}
	else if ( token[0] == '^' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '^='"));
			token += '=';
			++comp.pos;
		}
		else
		{
			D_TOKEN(Log("token '&'"));
		}
	}
	else if ( token[0] == '|' )
	{
		if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '|' )
		{
			D_TOKEN(Log("token '||'"));
			token += '|';
			++comp.pos;
		}
		else if ( (comp.pos < comp.source.size()) && comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '|='"));
			token += '=';
			++comp.pos;
		}
		else
		{
			D_TOKEN(Log("token '|'"));
		}
	}
	else if ( token[0] == '+' )
	{
		if ( comp.pos < comp.source.size() )
		{
			if ( comp.source[comp.pos] == '+' )
			{
				D_TOKEN(Log("token '++'"));
				token += '+';
				++comp.pos;
			}
			else if ( comp.source[comp.pos] == '=' )
			{
				D_TOKEN(Log("token '+='"));
				token += '=';
				++comp.pos;
			}
		}
		else
		{
			D_TOKEN(Log("token '+'"));
		}
	}
	else if ( token[0] == '-' )
	{
		if ( comp.pos < comp.source.size() )
		{
			if ( comp.source[comp.pos] == '-' )
			{
				D_TOKEN(Log("token '--'"));
				token += '-';
				++comp.pos;
			}
			else if ( comp.source[comp.pos] == '=' )
			{
				D_TOKEN(Log("token '--'"));
				token += '-';
				++comp.pos;
			}
		}
		else
		{
			D_TOKEN(Log("token '-'"));
		}
	}
	else if ( token[0] == '\"' )
	{
		token.clear();
		if ( !parseAsString(comp, token) )
		{
			return false;
		}

		value.type = CTYPE_STRING;
		value.string = token;
	}
	else if ( token[0] == '/' ) // might be a comment
	{
		if ( comp.pos >= comp.source.size() )
		{
			comp.err.format( "unexpected EOF" );
			return false; // not EOF but about to be..
		}

		if ( comp.source[comp.pos] == '/' ) // single line comment
		{
			++comp.pos; // skip the '/'

			for(;;)	// clear to EOL and re-read
			{
				if ( comp.pos >= comp.source.size() )
				{
					return true;
				}

				if ( comp.source[comp.pos++] == '\n' )
				{
					return getToken( comp, token, value );
				}
			}
		}

		if ( comp.source[comp.pos] == '*' )
		{
			++comp.pos; // skip it

			for(;;)	// clear to EOL and re-read
			{
				if ( (comp.pos + 1) >= comp.source.size() )
				{
					comp.err.format( "unexpected EOF" );
					return false;
				}

				if ( comp.source[comp.pos++] == '*' && comp.source[comp.pos++] == '/' )
				{
					D_TOKEN(Log("end of block comment"));
					return getToken( comp, token, value );
				}
			}
		}

		if ( comp.source[comp.pos] == '=' )
		{
			D_TOKEN(Log("token '/='"));
			token += '=';
			++comp.pos;
		}

		// bare '/' 
	}
	else if ( isdigit(token[0]) )
	{
		if ( comp.pos >= comp.source.size() )
		{
			Log( "<1> early termination" );
			return false;
		}

		if ( comp.source[comp.pos] == 'x' ) // interpret as hex
		{
			token += 'x';
			comp.pos++;

			for(;;)	// clear to EOL and re-read
			{
				if ( (comp.pos + 1) >= comp.source.size() )
				{
					Log( "<2> early termination" );
					return false;
				}

				if ( !isxdigit(comp.source[comp.pos]) )
				{
					break;
				}

				token += comp.source[comp.pos++];
			}

			if ( token.size() <= 18 )
			{
				value.type = CTYPE_INT;
				value.i64 = (int64_t)strtoull( Cstr(token).c_str(2), 0, 16 );
			}
			else
			{
				comp.err.format( "'0x' specifier overflowed [%s]", token.c_str() );
				return false;
			}
		}
		else if ( comp.source[comp.pos] == 'b' ) // interpret as hex
		{
			token += 'b';
			comp.pos++;

			for(;;)	// clear to EOL and re-read
			{
				if ( (comp.pos + 1) >= comp.source.size() )
				{
					comp.err.format( "unexpected EOF" );
					return false;
				}

				if ( (comp.source[comp.pos] != '0') && (comp.source[comp.pos] != '1') )
				{
					break;
				}

				token += comp.source[comp.pos++];
			}

			if ( token.size() <= 66 )
			{
				value.type = CTYPE_INT;
				value.i64 = 0;
				for( unsigned int i=2; i<token.size(); ++i )
				{
					value.i64 <<= 1;
					value.i64 |= (token[i] == '0') ? 0 : 1;
				}
			}
			else
			{
				comp.err.format( "'0b' specifier overflowed [%s]", token.c_str() );
				return false;
			}
		}
		else
		{
			bool decimal = false;
			for(;;)	// clear to EOL and re-read
			{
				if ( (comp.pos + 1) >= comp.source.size() )
				{
					comp.err.format( "unexpected EOF" );
					return false;
				}

				if ( comp.source[comp.pos] == '.' )
				{
					if ( decimal )
					{
						return false;
					}

					decimal = true;
				}
				else if ( !isdigit(comp.source[comp.pos]) )
				{
					break;
				}

				token += comp.source[comp.pos++];
			}

			if ( decimal )
			{
				value.type = CTYPE_FLOAT;
				value.d = atof( Cstr(token) );
			}
			else
			{
				value.type = CTYPE_INT;
				value.i64 = (int64_t)strtoll( Cstr(token), 0, 10 );
			}
		}
	}
	else if ( isalpha(token[0]) || token[0] == '_' ) // must be a label
	{
		for( ; comp.pos < comp.source.size() ; ++comp.pos )
		{
			if ( !isalnum(comp.source[comp.pos]) && comp.source[comp.pos] != '_' )
			{
				break;
			}

			token += comp.source[comp.pos];
		}
	}

	value.spaceAfter = (comp.pos < comp.source.size()) && isspace(comp.source[comp.pos]);
	
	D_TOKEN(Log("token[%s] %s", token.c_str(), (value.type == CTYPE_NULL) ? "" : formatToken(value).c_str()));

	return true;
}

