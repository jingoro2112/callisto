#ifndef CALLISTO_JSON_PARSER_H
#define CALLISTO_JSON_PARSER_H
/*------------------------------------------------------------------------------*/

#include "c_str.h"
#include "c_linkhash.h"
#include "c_hash.h"

namespace Callisto
{

//------------------------------------------------------------------------------
/*

Json value represents a Json-formatted object which includes values lists and objects
values: string/int/float/list/object
lists: list of objects by index, zero-based
objects: key/value keys must be strings, values can be anything

Adding example:

JsonValue value;
value["key"] = "value";
value["floatval"] = 1.45;

JsonValue &subObject = value["subobj"].object();
subObject["head"] = 10;
subObject["foot"] = 20;
subObject["name"] = "bob";

JsonValue &subList = subObject["a list"].list();
subList += 10;
subList += 20;
subList += 30;

produces:
{ "subobj" : {"a list":[10,20,30],"name":"bob","foot":20,"head":10},
"2" : 1.450000,
"key" : "value" }

Parsing example:

JsonValue parse( <the above string> );

printf("key

*/

#define D_JSONPARSER(a) //a

//------------------------------------------------------------------------------
class CCJsonValue
{
public:

	static inline void UTF8toJsonUnicode( const char* in, C_str& out );
	static inline int strJsonUnicodeToUTF8( const char* in, C_str& out );
	static inline int unicodeToUTF8( const unsigned int unicode, char* out, const unsigned int maxLen );

	inline int parse( const char* buf, bool clearFirst =true );

	// indent level of -1 means "no whitespace at all" so it's compact,
	// set it to 0 for human-readable
	inline C_str& write( C_str& str, int indentLevel =-1 ) const;
	inline C_str stringify( int indentLevel =-1 ) const { C_str out; write(out, indentLevel); return out; }

	// return the type of value this key is
	enum types
	{
		JNULL = 0,
		JBOOL,
		JINT,
		JSTRING,
		JFLOAT,
		JOBJECT,
		JLIST,
	};
	const int type() const { return m_type; }

	// for objects, return the current key, for all other types this operation is not defined
	const C_str& key() const { return m_key; }

	// for iterating through a list of objects when the keys are not known.
	//NOTE: iteration order is not defined, only guaranteed to visit each key exactly once.
	bool getObjectIterator( CCLinkHash<CCJsonValue>::Iterator &iter ) { if ( m_values ) { iter.iterate(*m_values); return true; } return false; }

	CCLinkHash<CCJsonValue>::Iterator begin() const { return m_values ? m_values->begin() : CCLinkHash<CCJsonValue>::Iterator(); }
	CCLinkHash<CCJsonValue>::Iterator end() const { return m_values ? m_values->end() : CCLinkHash<CCJsonValue>::Iterator(); }

	// convert to a string if it's not
	C_str& string() { m_type = JSTRING; return m_str; }
	C_str& format( const char* pattern, ... ) { m_type = JSTRING; m_str.clear(); va_list arg; va_start( arg, pattern ); m_str.appendFormatVA( pattern, arg ); va_end( arg ); return m_str; }

	inline void clear();
	CCJsonValue& null() { clear(); return *this; }
	const bool isNull() const { return m_type == JNULL; }

	const bool exists() const { return this != &m_deadValue; }
	const bool exists( const char* key ) const { return exists() && m_values->get( hash32(key,strlen(key)) ); }
	const bool exists( const int index ) const { return exists() && m_values->get( index ); }

	// does not validate that this object IS what is being requested,
	// asking for a value of the wrong type is undefined
	operator const char*() { return string().c_str(); }
	operator const C_str&() { return string(); }
	operator const int() { return (int)m_longlong; }
	operator const unsigned int() { return (unsigned int)m_longlong; }
	operator const long long() { return m_longlong; }
	operator const unsigned long long() { return (unsigned long long)m_longlong; }
	operator const float() { return (float)m_double; }
	operator const double() { return m_double; }
	operator const bool() { return m_longlong == 0 ? false : true; }

	// if this is a complex type (list or object), how many elements are in them
	const unsigned int numElements() const { return m_values ? m_values->count() : 0; }

	// convert this value to the requested object, clear it, and return
	CCJsonValue& object() { if ( m_type != JOBJECT ) { m_type = JOBJECT; createValues(); } return *this; }
	CCJsonValue& list() { if ( m_type != JLIST ) { m_type = JLIST; createValues(); } return *this; }
	CCJsonValue& sort( int(*compareFunc)(const CCJsonValue& item1, const CCJsonValue& item2) ) { if (m_values) { m_values->sort(compareFunc); } return *this; }

	// strings are keys(objects), integers are indexes(lists) if the
	// item does not exist, the 'special' index of -1 for lists returns the next unconsumed index
	inline CCJsonValue& operator[]( const char* key );
	inline CCJsonValue& operator[]( const int index );
	inline const CCJsonValue& operator[]( const char* key ) const;
	inline const CCJsonValue& operator[]( const int index ) const;

	inline bool operator==( const C_str& val  ) const { return (m_type == JSTRING) && (m_str == val); }
	inline bool operator==( const char* val ) const { return ((m_type == JSTRING) && (m_str == val)) || ((val == 0) && (m_type == JNULL)); }
	inline bool operator==( const long long val ) const { return (m_type == JINT) && (m_longlong == val); }
	inline bool operator==( const unsigned long long val ) const { return (m_type == JINT) && (m_longlong == (long long)val); }
	inline bool operator==( const bool val ) const { return (m_type == JBOOL) && ((m_longlong != 0) == val); }
	inline bool operator==( const double val ) const { return (m_type == JFLOAT) && (m_double == val); }
	inline bool operator==( const float val ) const { return (m_type ==  JFLOAT) && (m_double == (double)val); }
	inline bool operator==( const int val ) const { return (m_type == JINT) && (m_longlong == (long long)val); }
	inline bool operator==( const unsigned int val ) const { return (m_type == JINT) && (m_longlong == (long long)val); }
	inline bool operator==( const long val ) const { return (m_type == JINT) && (m_longlong == (long long)val); }
	inline bool operator==( const unsigned long val ) const { return (m_type == JINT) && (m_longlong == (long long)val); }
	inline bool operator!=( const C_str& val  ) const { return !(*this == val); }
	inline bool operator!=( const char* val ) const { return !(*this == val); }
	inline bool operator!=( const long long val ) const { return !(*this == val); }
	inline bool operator!=( const unsigned long long val ) const { return !(*this == val); }
	inline bool operator!=( const bool val ) const { return !(*this == val); }
	inline bool operator!=( const double val ) const { return !(*this == val); }
	inline bool operator!=( const float val ) const { return !(*this == val); }
	inline bool operator!=( const int val ) const { return !(*this == val); }
	inline bool operator!=( const unsigned int val ) const { return !(*this == val); }
	inline bool operator!=( const long val ) const { return !(*this == val); }
	inline bool operator!=( const unsigned long val ) const { return !(*this == val); }

	inline CCJsonValue& operator=( const C_str& val  ) { return operator=(val.c_str()); }
	inline CCJsonValue& operator=( const char* val );
	inline CCJsonValue& operator=( const long long val );
	inline CCJsonValue& operator=( const unsigned long long val ) { return operator=((long long)val); }
	inline CCJsonValue& operator=( const bool val );
	inline CCJsonValue& operator=( const double val );
	inline CCJsonValue& operator=( const float val ) { return operator=((double)val); }
	inline CCJsonValue& operator=( const int val ) { return operator=((long long)val); }
	inline CCJsonValue& operator=( const unsigned int val ) { return operator=((long long)val); }
	inline CCJsonValue& operator=( const long val ) { return operator=((long long)val); }
	inline CCJsonValue& operator=( const unsigned long val ) { return operator=((long long)val); }

	CCJsonValue() { m_values = 0; null(); }
	CCJsonValue( C_str const& data ) { m_values = 0; null(); parse(data); }
	CCJsonValue( const char* data ) { m_values = 0; null(); parse(data); }
	CCJsonValue( const CCJsonValue &other ) { C_str w; other.write(w); parse(w); }
	CCJsonValue& operator= (const CCJsonValue& other ) { C_str w; other.write(w); parse(w); if ( m_values) { m_values->reverseIterationOrder(); } return *this; }

	~CCJsonValue() { clear(); }

private:

	static CCJsonValue m_deadValue;

	inline void indent( C_str& str, int level ) const;
	
	operator char*() const { return 0; }
	
	inline int readRawValue( const char* buf );
	
	void createValues() { if (!m_values) m_values = new CCLinkHash<CCJsonValue>; m_values->clear(); }

	C_str m_str;
	long long m_longlong;
	double m_double;
	int m_type;
	
	C_str m_key;
	int m_index;
	CCLinkHash<CCJsonValue> *m_values;
};

//------------------------------------------------------------------------------
void CCJsonValue::clear()
{
	m_type = JNULL;
	if ( m_values )
	{
		delete m_values;
		m_values = 0;
	}
	m_index = 0;
	m_str.clear();
}

//------------------------------------------------------------------------------
const CCJsonValue& CCJsonValue::operator[]( const char* key ) const
{
	D_JSONPARSER(printf("operator[ \"%s\" ]\n", key ));

	if ( !exists() )
	{
		return m_deadValue.null();
	}
	else if ( m_type != JOBJECT )
	{
		// only objects can have string keys
		return m_deadValue.null();
	}

	unsigned int hash = hash32(key, strlen(key));
	CCJsonValue *V = m_values->get( hash );
	return V ? *V : m_deadValue.null();
	
}

//------------------------------------------------------------------------------
const CCJsonValue& CCJsonValue::operator[]( const int index ) const
{
	D_JSONPARSER(printf("operator[ %d ]\n", index ));

	if ( !exists() )
	{
		return m_deadValue.null();
	}

	CCJsonValue *V = m_values->get( index );
	return V ? *V : m_deadValue.null();
}

//------------------------------------------------------------------------------
CCJsonValue& CCJsonValue::operator[]( const char* key )
{
	D_JSONPARSER(printf("operator[ \"%s\" ]\n", key ));

	if ( !exists() )
	{
		return null();
	}
	else if ( m_type == JNULL )
	{
		object();
	}
	else if ( m_type != JOBJECT )
	{
		// only objects can have string keys
		return m_deadValue.null();
	}

	unsigned int hash = hash32(key,strlen(key));
	CCJsonValue *V = m_values->get( hash );

	if ( !V )
	{
		D_JSONPARSER(printf("adding key[%s]\n",key));
		V = m_values->add( hash );
		V->null();
		m_values->firstLinkToLast();
		
		V->m_key = key;
	}

	return V ? *V : m_deadValue.null();
}

//------------------------------------------------------------------------------
CCJsonValue& CCJsonValue::operator[]( const int index )
{
	D_JSONPARSER(printf("operator[ %d ]\n", index ));

	if ( !exists() )
	{
		return null();
	}
	
	if ( index == -1 )
	{
		return (*this)[ m_index ];
	}
	
	if ( m_type == JNULL )
	{
		list();
	}
	else if ( m_type != JLIST )
	{
		return m_deadValue.null(); // only lists can have numeric keys (indexes)
	}

	CCJsonValue *V = m_values->get( index );
	if ( !V )
	{
		V = m_values->add( index );
		V->null();

		m_values->firstLinkToLast();

		m_index = index + 1; // make this the new base, no other behavior really available
	}

	return V ? *V : m_deadValue.null();
}

//------------------------------------------------------------------------------
CCJsonValue& CCJsonValue::operator=( const char* val )
{
	if ( !exists() )
	{
		return null();
	}

	if ( m_values )
	{
		delete m_values;
		m_values = 0;
	}

	if ( !val )
	{
		D_JSONPARSER(printf("equating string NULL\n"));
		return null();
	}
	
	m_type = JSTRING;
	m_str = val;

	D_JSONPARSER(printf("equating string[%s]\n", val));
	
	return *this;
}

//------------------------------------------------------------------------------
CCJsonValue& CCJsonValue::operator=( const long long val )
{
	if ( !exists() )
	{
		return null();
	}

	if ( m_values )
	{
		delete m_values;
		m_values = 0;
	}
	m_type = JINT;
	m_str.format("%lld", val );
	m_longlong = val;
	D_JSONPARSER(printf("equating long long[%lld]\n", val));
	return *this;
}

//------------------------------------------------------------------------------
CCJsonValue& CCJsonValue::operator=( const bool val )
{
	if ( !exists() )
	{
		return null();
	}

	if ( m_values )
	{
		delete m_values;
		m_values = 0;
	}
	
	m_type = JBOOL;
	m_str.format("%s", val ? "true":"false" );
	m_longlong = val ? 1 : 0;
	D_JSONPARSER(printf("equating bool [%s]\n", val?"true":"false"));
	
	return *this;
}

//------------------------------------------------------------------------------
CCJsonValue& CCJsonValue::operator=( const double val )
{
	if ( !exists() )
	{
		return null();
	}

	if ( m_values )
	{
		delete m_values;
		m_values = 0;
	}

	m_type = JFLOAT;
	m_str.format("%g", val );
	m_double = val;
	m_longlong = (long long)val;
	D_JSONPARSER(printf("equating long long[%g]\n", val));
	
	return *this;
}

//------------------------------------------------------------------------------
int CCJsonValue::parse( const char* buf, bool clearFirst )
{
	if ( !exists() )
	{
		return 0;
	}

	if ( clearFirst )
	{
		clear();
	}
	
	const char* start = buf;
	bool valueComplete = false;
	bool inList = false;
	bool inObject = false;
	
	do
	{
		for( ; *buf && isspace(*buf); buf++ );

		if ( !(*buf) )
		{
			D_JSONPARSER(printf("returning INVALID<11>\n"));
			null();
			return 0;
		}

		switch( *buf )
		{
			case ']':
			{
				if ( !inList )
				{
					D_JSONPARSER(printf("returning INVALID<10>\n"));
					return 0;
				}
				buf++;
				valueComplete = true;
				m_values->reverseIterationOrder();
				break;
			}

			case '}':
			{
				if ( !inObject )
				{
					null();
					D_JSONPARSER(printf("returning INVALID<9>\n"));
					return 0;
				}
				buf++;
				valueComplete = true;
				m_values->reverseIterationOrder();
				break;
			}

			case '[':
			{
				if ( inList || inObject )
				{
					null();
					D_JSONPARSER(printf("returning INVALID<8>\n"));
					return 0;
				}
				
				createValues();
				inList = true;
				buf++;

				m_str.clear();
				m_type = JLIST;
				for( ; *buf && isspace(*buf); buf++ );

				if ( *buf == ']' ) // empty
				{
					buf++;
					valueComplete = true;
					break;
				}

				goto AddElement;
			}

			case '{':
			{
				if ( inList || inObject )
				{
					null();
					D_JSONPARSER(printf("returning INVALID<7>\n"));
					return 0;
				}

				createValues();
				inObject = true;
				buf++;

				m_str.clear();
				m_type = JOBJECT;
				for( ; *buf && isspace(*buf); buf++ );

				if ( *buf == '}' ) // empty
				{
					buf++;
					valueComplete = true;
					break;
				}

				goto AddElement;
			}

			case ',':
			{
				buf++;
AddElement:
				if ( inList )
				{
					D_JSONPARSER(printf("add element list 1\n"));

					// expect the item
					CCJsonValue* val = m_values->add( m_index++ );

					m_values->firstLinkToLast();

					D_JSONPARSER(printf("parsing from[%s]\n", buf ));

					int bytes = val->parse( buf );
					if ( !bytes )
					{
						null();
						D_JSONPARSER(printf("returning INVALID<6>\n"));
						return 0;
					}
					buf += bytes;
				}
				else if ( inObject )
				{
					// expect the key
					int read = readRawValue(buf);
					m_type = JOBJECT; // clobber what readRawValue() filled in
					
					if ( !read )
					{
						null();
						D_JSONPARSER(printf("returning INVALID<5>\n"));
						return 0;
					}
					buf += read;

					for( ; *buf && isspace(*buf); buf++ );
					if ( *buf != ':' )
					{
						null();
						D_JSONPARSER(printf("returning INVALID<4>\n"));
						return 0;
					}
					buf++; // skip marker

					CCJsonValue* val = m_values->add( hash32(m_str, m_str.size()) );

					m_values->firstLinkToLast();

					val->m_key = m_str;
					int bytes = val->parse( buf );
					if ( !bytes )
					{
						null();
						D_JSONPARSER(printf("returning INVALID<3>\n"));
						return 0;
					}
					buf += bytes;
				}
				else
				{
					D_JSONPARSER(printf("returning INVALID<2>\n"));
					return 0;
				}
				
				break;
			}

			default:
			{
				D_JSONPARSER(printf("reading raw value @[%s]\n", buf ));

				int bytes = readRawValue( buf );
				if ( !bytes )
				{
					null();
					D_JSONPARSER(printf("returning INVALID<1>\n"));
					return 0;
				}
				buf += bytes;

				for( ; *buf && isspace(*buf); buf++ );

				if ( *buf == ':' )
				{
					// I am a key, now get whatever I'm the key FOR
					buf++;
				}
				else
				{
					valueComplete = true;
				}

				break;
			}
		}

	} while( !valueComplete );

	D_JSONPARSER(printf("returning type[%d]\n", m_type));
	
	return (int)(buf - start);
}

//------------------------------------------------------------------------------
void CCJsonValue::indent( C_str& str, int indentLevel ) const
{
	for( int i=0; i<indentLevel; i++ )
	{
		str += '\t';
	}
}

//------------------------------------------------------------------------------
C_str& CCJsonValue::write( C_str& str, int indentLevel /*=-1*/ ) const
{
	if ( !exists() )
	{
		return str;
	}

	bool comma = false;
	switch( m_type )
	{
		case JLIST:
		{
			if ( indentLevel >= 0 )
			{
				str += "\n";
				indent( str, indentLevel++ );
				str += "[\n";
			}
			else
			{
				str += "[";
			}
			
			if ( m_values )
			{
				for( CCJsonValue *V = m_values->getFirst(); V; V = m_values->getNext() )
				{
					if ( comma ) // only insert a comma for the 2nd through nth
					{
						if ( indentLevel >= 0 )
						{
							str += ",\n";
							indent( str, indentLevel );
						}
						else
						{
							str += ',';
						}
					}
					else if ( indentLevel >= 0 )
					{
						str += '\n';
						indent( str, indentLevel );
					}
					comma = true;
					V->write( str, indentLevel );
				}
			}

			if ( indentLevel > 0 )
			{
				str += '\n';
				indent( str, --indentLevel );
				str += "]";
			}
			else
			{
				str += ']';
			}
			break;
		}

		case JOBJECT:
		{
			D_JSONPARSER(printf("WRITE:OBJECT\n"));

			if ( indentLevel >= 0 )
			{
				str += "\n";
				indent( str, indentLevel++ );
				str += "{\n";
			}
			else
			{
				str += "{";
			}
			
			if ( m_values )
			{
				for( CCJsonValue *V = m_values->getFirst(); V; V = m_values->getNext() )
				{
					D_JSONPARSER(printf("KEY[%s] val[%s]\n", V->m_key.c_str(), (const char*)*V));

					if ( comma ) // only insert a comma for the 2nd through nth
					{
						if ( indentLevel >= 0 )
						{
							str += ",\n";
							indent( str, indentLevel );
						}
						else
						{
							str += ',';
						}
					}
					else if ( indentLevel >= 0 )
					{
						str += '\n';
						indent( str, indentLevel );
					}
					comma = true;
					str += '\"';
					UTF8toJsonUnicode( V->m_key, str );
					str += (indentLevel >= 0) ? "\" : ":"\":";
					V->write( str, indentLevel );
				}
			}

			if ( indentLevel > 0 )
			{
				str += '\n';
				indent( str, --indentLevel );
				str += "}";
			}
			else
			{
				str += '}';
			}
			break;
		}

		case JSTRING:
		{
			str += '\"';
			UTF8toJsonUnicode( m_str, str ); // strings are just written out
			str += '\"';
			break;
		}

		case JNULL:
		{
			str += "null";
			break;
		}
		
		default:
		{
			str += m_str;
			break;
		}
	}

	return str;
}

//------------------------------------------------------------------------------
int CCJsonValue::readRawValue( const char* buf )
{
	const char* start = buf;
	// expecting exactly and only a single value
	for( ; *buf && isspace(*buf); buf++ );

	m_str.clear();

	if ( *buf == '\"' )
	{
		buf++;
		int chars = strJsonUnicodeToUTF8( buf, m_str );
		if ( chars == -1 )
		{
			return -1;
		}

		buf += chars + 1; // skip string and closing quotes
		m_type = JSTRING;

		D_JSONPARSER(printf("read raw string[%s]\n", m_str.c_str()));
	}
	else
	{
		int pos = 0;
		bool floatingpoint = false;
		bool exp = false;
		for( ; *buf ; buf++, pos++ )
		{
			if ( *buf == '.' )
			{
				floatingpoint = true;
			}
			if ( tolower(*buf) == 'e' )
			{
				exp = true;
			}

			// reach a terminator?
			if ( isspace(*buf) || *buf == ']' || *buf == '}' || *buf == ',' )
			{
				break;
			}

			m_str += *buf;

			if ( m_str.size() == 4 )
			{
				if ( tolower(m_str[0]) == 't'
					 && tolower(m_str[1]) == 'r'
					 && tolower(m_str[2]) == 'u'
					 && tolower(m_str[3]) == 'e' )
				{
					m_type = JBOOL;
					m_longlong = 1;

					D_JSONPARSER(printf("read raw bool[%s]\n", m_str.c_str()));

					return (int)(buf - start) + 1;
				}
				else if ( tolower(m_str[0]) == 'n'
						  && tolower(m_str[1]) == 'u'
						  && tolower(m_str[2]) == 'l'
						  && tolower(m_str[3]) == 'l' )
				{
					m_type = JNULL;
					
					D_JSONPARSER(printf("read raw null[%s]\n", m_str.c_str()));

					return (int)(buf - start) + 1;
				}
			}
			else if ( m_str.size() == 5
					  && tolower(m_str[0]) == 'f'
					  && tolower(m_str[1]) == 'a'
					  && tolower(m_str[2]) == 'l'
					  && tolower(m_str[3]) == 's'
					  && tolower(m_str[4]) == 'e' )
			{
				m_type = JBOOL;
				m_longlong = 0;

				D_JSONPARSER(printf("read raw bool[%s]\n", m_str.c_str()));

				return (int)(buf - start) + 1;
			}
		}

		if ( floatingpoint || exp )
		{
			m_type = JFLOAT;
			m_double = atof( m_str );
			m_longlong = (long long)m_double;

			D_JSONPARSER(printf("read raw floatingpoint[%s]\n", m_str.c_str())); 
		}
		else
		{
			m_type = JINT;
#ifdef _WIN32
			m_longlong = _strtoi64( m_str, 0, 10);
#else
			m_longlong = strtoll( m_str, 0, 10);
#endif
			m_double = (double)m_longlong;

			D_JSONPARSER(printf("read raw long long[%s]\n", m_str.c_str()));
		}
	}

	return (int)(buf - start);
}

//------------------------------------------------------------------------------
int CCJsonValue::unicodeToUTF8( const unsigned int unicode, char* out, const unsigned int maxLen )
{
	if ( unicode <= 0x7f ) 
	{
		out[0] = (char)unicode;
		return 1;
	} 
	else if (unicode <= 0x7FF) 
	{
		if ( maxLen < 2 )
		{
			return -1;
		}

		
		out[0] = (char)(0xC0 | (0x1F & (unicode >> 6)));
		out[1] = (char)(0x80 | (0x3F & unicode));
		return 2;
	} 
	else if (unicode <= 0xFFFF) 
	{
		if ( maxLen < 3 )
		{
			return -1;
		}

		out[0] = (char)(0xE0 | (0x0F & (unicode >> 12)));
		out[1] = (char)(0x80 | (0x3F & (unicode >> 6)));
		out[2] = (char)(0x80 | (0x3F & unicode));
		return 3;
	}
	else if (unicode <= 0x10FFFF) 
	{
		if ( maxLen < 3 )
		{
			return -1;
		}
		
		out[0] = (char)(0xF0 | (0x07 & (unicode >> 18)));
		out[1] = (char)(0x80 | (0x3F & (unicode >> 12)));
		out[2] = (char)(0x80 | (0x3F & (unicode >> 6)));
		out[3] = (char)(0x80 | (0x3F & unicode));
		return 4;
	}

	return -1;
}

//------------------------------------------------------------------------------
void CCJsonValue::UTF8toJsonUnicode( const char* in, C_str& str )
{
	for( unsigned int pos = 0; in[pos]; pos++ )
	{
		unsigned int unicodeCharacter = 0;
		if ( in[pos] & 0x80 ) // marker for multi-byte
		{
			if ( (in[pos] & 0x40) ) // 0x11_xxxxx
			{
				if ( (in[pos] & 0x20) ) // 0x111_xxxx
				{
					if ( (in[pos] & 0x10) ) // 0x1111_xxx
					{
						if ( (in[pos] & 0x08) ) // 0x11111_xx
						{
							if ( (in[pos] & 0x04) // 0x111111_x
								 && !(in[pos] & 0x02) // 0x1111110x?
								 && in[pos+1] && ((in[pos+1] & 0xC0) == 0x80)
								 && in[pos+2] && ((in[pos+2] & 0xC0) == 0x80)
								 && in[pos+3] && ((in[pos+3] & 0xC0) == 0x80)
								 && in[pos+4] && ((in[pos+4] & 0xC0) == 0x80)
								 && in[pos+5] && ((in[pos+5] & 0xC0) == 0x80) )
							{
								// 0x1111110x : 0x10000000 : 0x10000000 : 0x10000000 : 0x10000000 : 0x10000000
								unicodeCharacter = (((unsigned int)in[pos]) & 0x01) << 31
												   | (((unsigned int)in[pos+1]) & 0x3F) << 25
												   | (((unsigned int)in[pos+2]) & 0x3F) << 18
												   | (((unsigned int)in[pos+3]) & 0x3F) << 12
												   | (((unsigned int)in[pos+4]) & 0x3F) << 6
												   | (((unsigned int)in[pos+5]) & 0x3F);
							}
							// 0x111110xx:
							else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80)  
									  && in[pos+2] && ((in[pos+2] & 0xC0) == 0x80)
									  && in[pos+3] && ((in[pos+3] & 0xC0) == 0x80)
									  && in[pos+4] && ((in[pos+4] & 0xC0) == 0x80) )
							{
								// 0x111110xx : 0x10000000 : 0x10000000 : 0x10000000 : 0x10000000
								unicodeCharacter = (((unsigned int)in[pos]) & 0x03) << 25
												   | (((unsigned int)in[pos+1]) & 0x3F) << 18
												   | (((unsigned int)in[pos+2]) & 0x3F) << 12
												   | (((unsigned int)in[pos+3]) & 0x3F) << 6
												   | (((unsigned int)in[pos+4]) & 0x3F);
							}
						}
						// 0x11110xxx:
						else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80) 
								  && in[pos+2] && ((in[pos+2] & 0xC0) == 0x80)
								  && in[pos+3] && ((in[pos+3] & 0xC0) == 0x80) )
						{
							// 0x11110xxx : 0x10000000 : 0x10000000 : 0x10000000
							unicodeCharacter = (((unsigned int)in[pos]) & 0x07) << 18
											   | (((unsigned int)in[pos+1]) & 0x3F) << 12
											   | (((unsigned int)in[pos+2]) & 0x3F) << 6
											   | (((unsigned int)in[pos+3]) & 0x3F);
						}
					}
					// 0x1110xxxx:
					else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80)
							  && in[pos+2] && ((in[pos+2] & 0xC0) == 0x80) )
					{
						// 0x1110xxxx : 0x10000000 : 0x10000000
						unicodeCharacter = (((unsigned int)in[pos]) & 0x0F) << 12
										   | (((unsigned int)in[pos+1]) & 0x3F) << 6
										   | (((unsigned int)in[pos+2]) & 0x3F);
					}
				}
				// 0x110xxxxx:
				else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80) )
				{
					// 0x110xxxxx : 0x10000000
					unicodeCharacter = (((unsigned int)in[pos]) & 0x1F) << 6
									   | (((unsigned int)in[pos+1]) & 0x7F);
				}
			}

			if ( unicodeCharacter )
			{
				if ( (unicodeCharacter & 0x0000FFFF) != unicodeCharacter )
				{
					// TODO: figure out how to emit two UTF-16s based on the UTF-8 we
					// are decoding if it takes more than 16 bits;
					// "http://www.ietf.org/rfc/rfc4627.txt"
					//
					// To escape an extended character that is not in the Basic Multilingual
					// Plane, the character is represented as a twelve-character sequence,
					// encoding the UTF-16 surrogate pair.  So, for example, a string
					// containing only the G clef character (U+1D11E) may be represented as
					// "\uD834\uDD1E"
					//
					// ref: http://www.russellcottrell.com/greek/utilities/SurrogatePairCalculator.htm
				}

				str.appendFormat( "\\u%04X", unicodeCharacter & 0x0000FFFF );
			}
		}
		else
		{
			switch( in[pos] )
			{
				case '\"': str += '\\'; str += '\"'; break;
				case '\\': str += '\\'; str += '\\'; break;
				case '\b': str += '\\'; str += 'b'; break;
				case '\f': str += '\\'; str += 'f'; break;
				case '\n': str += '\\'; str += 'n'; break;
				case '\r': str += '\\'; str += 'r'; break;
				case '\t': str += '\\'; str += 't'; break;
				case '/': str += '\\'; str += '/'; break;
				default: str += in[pos]; break;
			}
		}
	}
}

//------------------------------------------------------------------------------
int CCJsonValue::strJsonUnicodeToUTF8( const char* in, C_str& out )
{
	int pos = 0;
	for( pos=0; in[pos]; pos++ )
	{
		if ( in[pos] != '\\' )
		{
			if ( in[pos] == '\"' )
			{
				return pos;
			}

			out += in[pos];
		}
		else
		{
			pos++;
			switch( in[pos] )
			{
				case 0:
				{
					return pos;
				}

				case '\"':
				case '\\':
				case '/':
				{
					out += in[pos];
					break;
				}

				case 'b':
				{
					out += '\b';
					break;
				}

				case 't':
				{
					out += '\t';
					break;
				}

				case 'n':
				{
					out += '\n';
					break;
				}

				case 'r':
				{
					out += '\r';
					break;
				}

				case 'f':
				{
					out += '\f';
					break;
				}

				case 'u':
				{
					// pull out the first HEX code
					unsigned int u = 0;
					pos++;
					for ( int i=0; i<4; pos++, i++ )
					{
						if ( !in[pos] )
						{
							return -1;
						}

						char c = in[pos];
						if ( c >= '0'  &&  c <= '9' )
						{
							u = (u << 4) + (c - '0');
						}
						else if ( c >= 'a'  &&  c <= 'f' )
						{
							u = (u << 4) + ((c - 'a') + 10);
						}
						else if ( c >= 'A'  &&  c <= 'F' )
						{
							u = (u << 4) + ((c - 'A') + 10);
						}
						else
						{
							return -1;
						}
					}

					// if this one indicates it is part of a surrogate
					// pair, pull the next one and mix it
					if ( (u >= 0xD800) && (u <= 0xDBFF) )
					{
						if ( (in[pos] != '\\') ||(in[pos+1] != 'u') )
						{
							return -1;
						}
						pos += 2;

						unsigned int p = 0;
						for ( int i=0; i<4; pos++, i++ )
						{
							if ( !in[pos] )
							{
								return -1;
							}

							char c = in[pos];
							if ( c >= '0'  &&  c <= '9' )
							{
								p = (p << 4) + (c - '0');
							}
							else if ( c >= 'a'  &&  c <= 'f' )
							{
								p = (p << 4) + ((c - 'a') + 10);
							}
							else if ( c >= 'A'  &&  c <= 'F' )
							{
								p = (p << 4) + ((c - 'A') + 10);
							}
							else
							{
								return -1;
							}
						}
				
						u = 0x10000 + ((u & 0x3FF) << 10) + (p & 0x3FF);
					}
					
					char str[8];
					int written = unicodeToUTF8( u, str, 8 );
					if ( written == -1 )
					{
						return -1;
					}
					out.append( str, written );
					break;
				}
			}
		}
	}

	return pos;
}

}

#endif

