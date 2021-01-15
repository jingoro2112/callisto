#ifndef CALLISTO_STR_H
#define CALLISTO_STR_H
/* ------------------------------------------------------------------------- */

#if defined(_WIN32) && !defined(__MINGW32__)
#pragma warning (disable : 4996) // remove Windows nagging about not using their proprietary interfaces
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <assert.h>

namespace Callisto
{

/*

 - No default dynamic representation. Put a C/W/Tstr on the stack without
 sweating what the constructor will go off and do. Until the
 string gets larger than this, after which heap load must be imposed: */
const unsigned int c_csizeofBaseString = 31;

const int c_ccstrFormatBufferSize = 32000;

//-----------------------------------------------------------------------------
template<class C> class CTstr
{
public:
	CTstr<C>() { m_len = 0; m_smallbuf[0] = 0; m_str = m_smallbuf; m_buflen = c_csizeofBaseString; }
	CTstr<C>( const CTstr<C>& str) { m_len = 0; m_str = m_smallbuf; m_buflen = c_csizeofBaseString; set(str, str.size()); } 
	CTstr<C>( const C* s, const unsigned int len =-1 ) { m_len = 0; m_str = m_smallbuf; m_buflen = c_csizeofBaseString; set(s, len); }
	CTstr<C>( const C c) { m_len = 1; m_str = m_smallbuf; m_smallbuf[0] = c; m_smallbuf[1] = 0; m_buflen = c_csizeofBaseString; } 
	CTstr<C>( const int len ) { alloc(len); }
	CTstr<C>( const unsigned int len ) { alloc(len); }
	~CTstr<C>() { if ( m_str != m_smallbuf ) delete[] m_str; }

	inline CTstr<C>& clear( const bool deleteMem =false ) { if ( deleteMem ) { release(); } else { m_len = 0; m_str[0] = 0; } return *this; }

	// sets this string to be the size requested, regardless of contents
	CTstr<C>& setSize( const unsigned int size, const bool preserveContents =true, const bool zeroFill =false ) { alloc(size, preserveContents, zeroFill); m_len = size; m_str[size] = 0; return *this; }
	
	inline CTstr<C>& alloc( const unsigned int characters, const bool preserveContents =true, const bool zeroFill =false ); 

	unsigned int size() const { return m_len; } // see length
	unsigned int length() const { return m_len; } // see size
	unsigned int sizeInCore() const { return m_len*sizeof(C); } // how many bytes this class takes up

	inline CTstr<C>& giveOwnership( C* dynamicMemory, const unsigned int bufferlen ); // give this class ownership of some dynamic memory
	inline unsigned int release( C** toBuf =0 ); // release the internal buffer, return value must be manually delete[]ed !!

	const C* c_str( const unsigned int offset =0 ) const { assert(offset <= m_len); return m_str + offset; }
	C* p_str( const unsigned int offset =0 ) const { assert(offset <= m_len); return m_str + offset; }
	unsigned char* b_str( const unsigned int offset =0 ) const { assert(offset <= m_len); return (unsigned char *)m_str + offset; }
	operator const void*() const { return m_str; }
	operator const C*() const { return m_str; }

	// file operations
	inline bool fileToBuffer( const char* fileName, const bool appendToBuffer =false );
	inline bool bufferToFile( const char* fileName, const bool append =false ) const;

	inline CTstr<C>& set( const C* buf, const unsigned int len =(unsigned int)-1 ) { m_len = 0; m_str[0] = 0; return insert( buf, len ); }
	CTstr<C>& set( const CTstr& str ) { return set( str.m_str, str.m_len ); }

	inline CTstr<C>& trim();
	inline CTstr<C>& ltrim();
	inline CTstr<C>& rtrim();
	inline CTstr<C>& truncate( const unsigned int newLen ); // reduce size to 'newlen'
	CTstr<C>& shave( const unsigned int e ) { return (e > m_len) ? clear() : truncate(m_len - e); } // remove 'x' trailing characters

	inline CTstr<C> subStr( const unsigned int from, const unsigned int to ) const;

	// memmove from sourcePosition to end of string down to the start of the string
	inline CTstr<C>& shift( const unsigned int fromSourcePosition, const unsigned int toTargetPos =0 );
	inline unsigned int dequeue( const unsigned int bytes, CTstr *str =0 ); // returns characters actually dequeued (may be zero)

	inline CTstr<C>& escape( const C* chars, CTstr<C>* target =0, const C escapeCharacter ='\\' );
	inline CTstr<C>& unescape( const C escapeCharacter ='\\' );

	inline unsigned int replace( const C* oldstr, const C* newstr, const bool ignoreCase =false, const int occurances =0 );

	inline bool isMatch( const C* buf, const bool matchCase =true, const bool matchPartial =false ) const;

	inline CTstr<C>& toLower() { for( unsigned int i=0; i<m_len; i++ ) { m_str[i] = (C)tolower((char)m_str[i]); } return *this; }
	inline CTstr<C>& toUpper() { for( unsigned int i=0; i<m_len; i++ ) { m_str[i] = (C)toupper((char)m_str[i]); } return *this; }

	inline const C* find( const C* buf, const bool matchCase =true ) const;

	inline CTstr<C>& insert( const C* buf, const unsigned int len =(unsigned int)-1, const unsigned int startPos =0 );
	inline CTstr<C>& insert( const CTstr<C>& s, const unsigned int startPos =0 ) { return insert(s.m_str, s.m_len, startPos); }
	
	inline CTstr<C>& append( const C* buf, const unsigned int len =(unsigned int)-1 ) { return insert(buf, len, m_len); } 
	inline CTstr<C>& append( const C c );
	inline CTstr<C>& append( const CTstr<C>& s ) { return insert(s.m_str, s.m_len, m_len); }
	inline C* addTail() { alloc(m_len += 1); return m_str + (m_len - 1); }
	inline C* addHead() { alloc(m_len += 1); memmove(m_str + 1, m_str, (m_len - 1)*sizeof(C)); return m_str; }

	// define the usual suspects:
	
	const C& operator[]( const int l ) const { assert(m_len && ((unsigned int)l < m_len)); return m_str[l]; }
	const C& operator[]( const unsigned int l ) const { assert(m_len && (l < m_len)); return m_str[l]; }
	C& operator[]( const int l ) { assert(m_len && ((unsigned int)l < m_len)); return m_str[l]; }
	C& operator[]( const unsigned int l ) { assert(m_len && (l < m_len)); return m_str[l]; }

	CTstr<C>& operator += ( const CTstr<C>& str ) { return append(str.m_str, str.m_len); }
	CTstr<C>& operator += ( const C* s ) { return append(s); }
	CTstr<C>& operator += ( const C c ) { return append(c); }

	CTstr<C>& operator = ( const CTstr<C>& str ) { if ( &str != this ) set(str, str.size()); return *this; }
	CTstr<C>& operator = ( const CTstr<C>* str ) { if ( !str ) { clear(); } else if ( this != this ) { set(*str, str->size()); } return *this; }
	CTstr<C>& operator = ( const C* c ) { set(c); return *this; }
	CTstr<C>& operator = ( const C c ) { set(c); return *this; }

	friend bool operator != ( const CTstr<C>& s1, const CTstr<C>& s2 ) { return s1.m_len != s2.m_len || (memcmp(s1.m_str, s2.m_str, s1.m_len*sizeof(C)) != 0); }
	friend bool operator != ( const CTstr<C>& s, const C* z ) { return !s.isMatch( z ); }
	friend bool operator != ( const C* z, const CTstr<C>& s ) { return !s.isMatch( z ); }
	friend bool operator == ( const CTstr<C>& s1, const CTstr<C>& s2 ) { return s1.m_len == s2.m_len && (memcmp(s1.m_str, s2.m_str, s1.m_len*sizeof(C)) == 0); }
	friend bool operator == ( const C* z, const CTstr<C>& s ) { return s.isMatch( z ); }
	friend bool operator == ( const CTstr<C>& s, const C* z ) { return s.isMatch( z ); }

	friend CTstr<C> operator + ( const CTstr<C>& str, const C* s) { CTstr<C> T(str); T += s; return T; }
	friend CTstr<C> operator + ( const CTstr<C>& str, const C c) { CTstr<C> T(str); T += c; return T; }
	friend CTstr<C> operator + ( const C* s, const CTstr<C>& str ) { CTstr<C> T(s); T += str; return T; }
	friend CTstr<C> operator + ( const C c, const CTstr<C>& str ) { CTstr<C> T(c); T += str; return T; }
	friend CTstr<C> operator + ( const CTstr<C>& str1, const CTstr<C>& str2 ) { CTstr<C> T(str1); T += str2; return T; }

	class Iterator
	{
	public:
		Iterator( CTstr<C> const& S ) : m_S(&S), m_i(0) {}
		bool operator!=( const Iterator& other ) { return m_i != other.m_i; }
		C& operator* () const { return m_S->m_str[m_i]; }
		const Iterator operator++() { m_i++; return *this; }
		CTstr<C> const* m_S;
		int m_i;
	};

	Iterator begin() const { return Iterator(*this); }
	Iterator end() const { Iterator I(*this); I.m_i = this->m_len; return I; }

protected:

	operator C*() const { return m_str; } // prevent accidental use

	C *m_str; // first element so if the class is cast as a C and de-referenced it always works
	
	unsigned int m_buflen; // how long the buffer itself is
	unsigned int m_len; // how long the string is in the buffer
	C m_smallbuf[ c_csizeofBaseString + 1 ]; // small temporary buffer so a new/delete is not imposed for small strings
};

//------------------------------------------------------------------------------
class C_str : public CTstr<char>
{
public:
	C_str() {}
	C_str( const C_str* str ) { set(str ? *str : ""); }
	C_str( const wchar_t* buf ) { *this = buf; }
	C_str( const char* buf, const unsigned int len =(unsigned int)-1 ) : CTstr<char>(buf, len) {}
	C_str( const char c ) : CTstr<char>(c) {}

	C_str& operator = ( const wchar_t* s ) { if (s && (m_len = (unsigned int)wcstombs(0, s, 0)) != (unsigned int)-1) wcstombs( alloc((unsigned int)m_len + 1, false).p_str(), s, m_len + 1 ); else clear(); return *this; }
	C_str& operator = ( const char* c ) { set(c); return *this; }

	static C_str sprintf( const char* format, ... ) { C_str T; va_list arg; va_start(arg, format); T.formatVA(format, arg); va_end(arg); return T; }
		
	C_str& format( const char* format, ... ) { va_list arg; va_start( arg, format ); clear(); appendFormatVA( format, arg ); va_end( arg ); return *this; }
	C_str& formatVA( const char* format, va_list arg ) { clear(); return appendFormatVA(format, arg); }
	C_str& appendFormat( const char* format, ... ) { va_list arg; va_start( arg, format ); appendFormatVA( format, arg ); va_end( arg ); return *this; }
	C_str& appendFormatVA( const char* format, va_list arg )
	{
		char buf[c_ccstrFormatBufferSize + 1];
		int len = vsnprintf( buf, c_ccstrFormatBufferSize, format, arg );
		if ( len > 0 ) insert( buf, (unsigned int)len, m_len );
		return *this;
	}
};

//------------------------------------------------------------------------------
class W_str : public CTstr<wchar_t>
{
public:
	W_str() {}
	W_str( const W_str* str ) { set(str ? *str : L""); }
	W_str( const char* buf )	{ *this = buf; }
	W_str( const unsigned char* buf ) { *this = (char*)buf; }
	W_str( const wchar_t* buf, const unsigned int len =(unsigned int)-1 ) : CTstr<wchar_t>(buf, len) {}
	W_str( const wchar_t w ) : CTstr<wchar_t>(w) {}

	W_str& operator = ( const char* s ) { if ( s && (m_len = (unsigned int)mbstowcs(0, s, 0)) != (unsigned int)-1 ) mbstowcs( alloc((unsigned int)m_len + 1, false).p_str(), s, m_len + 1 ); else clear(); return *this; }
	W_str& operator = ( const wchar_t* w ) { set(w); return *this; }

	static W_str sprintf( const wchar_t* format, ... ) { W_str T; va_list arg; va_start(arg, format); T.formatVA(format, arg); va_end(arg); return T; }

	W_str& format( const wchar_t* format, ... ) { va_list arg; va_start( arg, format ); clear(); appendFormatVA( format, arg ); va_end( arg ); return *this; }
	W_str& formatVA( const wchar_t* format, va_list arg ) { clear(); return appendFormatVA(format, arg); }
	W_str& appendFormat( const wchar_t* format, ... ) { va_list arg; va_start( arg, format ); appendFormatVA( format, arg ); va_end( arg ); return *this; }
	W_str& appendFormatVA( const wchar_t* format, va_list arg )
	{
		wchar_t buf[c_ccstrFormatBufferSize + 1];
		int len = vswprintf( buf, c_ccstrFormatBufferSize, format, arg );
		if ( len > 0 ) insert( buf, (unsigned int)len, m_len );
		return *this;
	}
};

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::alloc( const unsigned int characters,
										   const bool preserveContents,
										   const bool zeroFill /*=false*/ )
{
	if ( characters >= m_buflen ) // only need to alloc if more space is requested than we have
	{
		C* neW_str = new C[ characters + 1 ]; // create the space
		if ( zeroFill )
		{
			memset( neW_str + (m_buflen*sizeof(C)), 0, ((characters*sizeof(C)) + sizeof(C)) - (m_buflen*sizeof(C)) );
		}
		else
		{
			neW_str[characters] = 0;
		}
		
		if ( preserveContents ) 
		{
			memcpy( neW_str, m_str, m_buflen * sizeof(C) ); // preserve whatever we had
		}
		
		if ( m_str != m_smallbuf )
		{
			delete[] m_str;
		}
		
		m_str = neW_str;
		m_buflen = characters;		
	}
	else if ( zeroFill && (m_str == m_smallbuf) )
	{
		memset( m_str + characters, 0, (m_buflen - characters) * sizeof(C) ); // make sure the static buffer is zeroed since it was never "allocated"
	}

	return *this;
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::giveOwnership( C* dynamicMemory, const unsigned int bufferlen )
{
	if ( bufferlen < c_csizeofBaseString )
	{
		set( dynamicMemory, bufferlen );
		delete[] dynamicMemory;
	}
	else
	{
		if ( m_str != m_smallbuf )
		{
			delete[] m_str;
		}
		m_str = dynamicMemory;
		m_buflen = bufferlen;
		m_len = bufferlen;
	}

	return *this;
}

//-----------------------------------------------------------------------------
template<class C> unsigned int CTstr<C>::release( C** toBuf )
{
	unsigned int retLen = size();

	if ( toBuf )
	{
		if ( m_str == m_smallbuf ) // there is no dynamic rep, create one
		{
			*toBuf = new C[m_len + 1];
			memcpy( *toBuf, m_str, (m_len+1)*sizeof(C) );
		}
		else // just give it up
		{
			*toBuf = m_str;
			m_str = m_smallbuf;
			m_buflen = c_csizeofBaseString;
		}
	}
	else if ( m_str != m_smallbuf )
	{
		delete[] m_str;
		m_str = m_smallbuf;
		m_buflen = c_csizeofBaseString;
	}

	m_len = 0;
	m_str[0] = 0;

	return retLen;
}

//-----------------------------------------------------------------------------
template<class C> bool CTstr<C>::fileToBuffer( const char* fileName, const bool appendToBuffer )
{
	if ( !fileName )
	{
		return false;
	}

#ifdef _WIN32
	struct _stat sbuf;
	int ret = _stat( fileName, &sbuf );
#else
	struct stat sbuf;
	int ret = stat( fileName, &sbuf );
#endif

	if ( ret != 0 )
	{
		return false;
	}

	FILE *infil = fopen( fileName, "rb" );
	if ( !infil )
	{
		return false;
	}
	
	if ( appendToBuffer )
	{
		alloc( sbuf.st_size + m_len, true );
		m_str[ sbuf.st_size + m_len ] = 0;
		ret = (int)fread( m_str + m_len, sbuf.st_size, 1, infil );
		m_len += sbuf.st_size;
	}
	else
	{
		alloc( sbuf.st_size, false );
		m_len = sbuf.st_size;
		m_str[ m_len ] = 0;
		ret = (int)fread( m_str, m_len, 1, infil );
	}

	fclose( infil );
	return ret == 1;
}

//-----------------------------------------------------------------------------
template<class C> bool CTstr<C>::bufferToFile( const char* fileName, const bool append) const
{
	if ( !fileName )
	{
		return false;
	}

	FILE *outfil = append ? fopen( fileName, "a+b" ) : fopen( fileName, "wb" );
	if ( !outfil )
	{
		return false;
	}

	int ret = (int)fwrite( m_str, m_len, 1, outfil );
	fclose( outfil );

	return (m_len == 0) || (ret == 1);
}

//-----------------------------------------------------------------------------
template<class C> unsigned int CTstr<C>::replace( const C* oldstr, const C* newstr, const bool ignoreCase /*=false*/, const int occurances /*=0*/ )
{
	if ( !oldstr || !newstr )
	{
		return 0;
	}

	int r = 0;

	CTstr newStr;
	newStr.alloc( m_len, false );

	for( unsigned int pos=0; pos<m_len; pos++ )
	{
		if ( *oldstr == m_str[pos] || (ignoreCase && toupper(*oldstr) == toupper(m_str[pos])))
		{
			unsigned int oldPos = pos;
			unsigned int i = 0;
			if ( ignoreCase )
			{
				for( ; oldstr[i] && pos<m_len; i++, pos++ )
				{
					if ( toupper(m_str[pos]) != toupper(oldstr[i]) )
					{
						break;
					}
				}
			}
			else
			{
				for( ; oldstr[i] && pos<m_len; i++, pos++ )
				{
					if ( m_str[pos] != oldstr[i] )
					{
						break;
					}
				}
			}

			if ( !oldstr[i] ) // match found
			{
				r++;
				pos--;
				newStr += newstr;

				if ( occurances && r >= occurances )
				{
					newStr += m_str + pos + 1;
					break;
				}
			}
			else
			{
				pos = oldPos;
				newStr += m_str[pos];
			}
		}
		else
		{
			newStr += m_str[pos];
		}
	}

	set( newStr );
	return r;
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::trim()
{
	unsigned int start = 0;

	// find start
	for( ; start<m_len && isspace( (unsigned char)*(m_str + start) ) ; start++ );

	// is the whole thing whitespace?
	if ( start == m_len )
	{
		clear();
		return *this;
	}

	// copy down the characters one at a time, noting the last
	// non-whitespace character position, which will become the length
	unsigned int pos = 0;
	unsigned int marker = start;
	for( ; start<m_len; start++,pos++ )
	{
		if ( !isspace((unsigned char)(m_str[pos] = m_str[start])) )
		{
			marker = pos;
		}
	}

	m_len = marker + 1;
	m_str[m_len] = 0;

	return *this;
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::rtrim()
{
	if ( !m_len )
	{
		return *this;
	}

	unsigned int start = m_len - 1;

	// find start
	for( ; start>0 && isspace( (unsigned char)*(m_str + start) ) ; --start );

	// is the whole thing whitespace?
	if ( start == (m_len - 1) )
	{
		return *this;
	}

	return truncate( start + 1 );
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::ltrim()
{
	unsigned int start = 0;

	// find start
	for( ; start<m_len && isspace( (unsigned char)*(m_str + start) ) ; ++start );

	if ( start )
	{
		return shift( start );
	}
	else
	{
		return *this;
	}
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::truncate( const unsigned int newLen )
{
	if ( newLen >= m_len )
	{
		return *this;
	}

	if ( newLen < c_csizeofBaseString )
	{
		if ( m_str != m_smallbuf )
		{
			m_buflen = c_csizeofBaseString;
			memcpy( m_smallbuf, m_str, newLen*sizeof(C) );
			delete[] m_str;
			m_str = m_smallbuf;
		}
	}

	m_str[ newLen ] = 0;
	m_len = newLen;

	return *this;
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C> CTstr<C>::subStr( const unsigned int from, const unsigned int to ) const
{
	CTstr<C> sub;
	if ( to > from && (from < m_len) && (to <= m_len) )
	{
		sub.set( m_str + from, to - from );
	}
	return sub;
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::shift( const unsigned int fromSourcePosition, const unsigned int toTargetPos /*=0*/ )
{
	if ( fromSourcePosition >= m_len )
	{
		clear();
	}
	else
	{
		unsigned int bytes = m_len - fromSourcePosition;

		if ( (bytes + toTargetPos) < c_csizeofBaseString )
		{
			if ( m_str != m_smallbuf )
			{
				if ( toTargetPos )
				{
					memcpy( m_smallbuf,
							m_str,
							toTargetPos*sizeof(C) );
				}

				memcpy( m_smallbuf + toTargetPos,
						m_str + fromSourcePosition,
						bytes*sizeof(C) );

				delete[] m_str;
				m_str = m_smallbuf;
				m_buflen = c_csizeofBaseString;
			}
			else
			{
				memmove( m_smallbuf + toTargetPos,
						 m_smallbuf + fromSourcePosition,
						 bytes*sizeof(C) );
			}
		}
		else
		{
			memmove( m_str + toTargetPos,
					 m_str + fromSourcePosition,
					 bytes*sizeof(C) );
		}

		m_len = bytes + toTargetPos;
		m_str[m_len] = 0;
	}

	return *this;
}

//-----------------------------------------------------------------------------
template<class C> unsigned int CTstr<C>::dequeue( const unsigned int bytes, CTstr* str /*=0*/ )
{
	int corrected = bytes > m_len ? m_len : bytes;
	
	if( str )
	{
		str->set( m_str, corrected );
	}
	
	shift( corrected );
	return corrected;
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::escape( const C* chars, CTstr<C>* target, const C escapeCharacter )
{
	CTstr<C> escapedString;

	for( unsigned int i=0 ; i<m_len ; i++ )
	{
		if ( m_str[i] == escapeCharacter )
		{
			escapedString.append( escapeCharacter );
			escapedString.append( escapeCharacter );
		}
		else
		{
			int j = 0;
			for( ; chars[j]; j++ )
			{
				if ( m_str[i] == chars[j] )
				{
					escapedString.append( escapeCharacter );
					escapedString.append( chars[j] );
					break;
				}
			}

			if ( !chars[j] )
			{
				escapedString.append( m_str[i] );
			}
		}
	}

	if ( target )
	{
		target->set( escapedString );
		return *target;
	}
	else
	{
		set( escapedString );
		return *this;
	}
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::unescape( const C escapeCharacter )
{
	if ( m_len <= 1 )
	{
		return *this;
	}

	unsigned int copyback = 0;
	for( unsigned int i=0 ; i<m_len; i++, copyback++ )
	{
		if ( m_str[i] == escapeCharacter )
		{
			i++;
		}

		m_str[copyback] = m_str[i];
	}

	m_str[copyback] = 0;
	truncate( copyback );

	return *this;
}

//-----------------------------------------------------------------------------
template<class C> bool CTstr<C>::isMatch( const C* buf, bool matchCase /*=true*/, bool matchPartial /*=false*/ ) const
{
	if ( !buf )
	{
		return m_len == 0;
	}

	if ( m_len == 0 )
	{
		return buf[0] == 0;
	}

	unsigned int i;
	if ( matchCase )
	{
		for( i=0; i<m_len; i++ )
		{
			if ( buf[i] == 0 )
			{
				return matchPartial;
			}

			if ( buf[i] != m_str[i] )
			{
				return false;
			}
		}
	}
	else
	{
		for( i=0; i<m_len; i++ )
		{
			if ( buf[i] == 0 )
			{
				return matchPartial;
			}

			if ( tolower(buf[i]) != tolower(m_str[i]) )
			{
				return false;
			}
		}
	}

	if ( buf[i] != 0 ) // wrong length
	{
		return matchPartial;
	}

	return true;
}

//-----------------------------------------------------------------------------
template<class C> const C* CTstr<C>::find( const C* buf, const bool matchCase ) const
{
	if ( !buf || !m_len )
	{
		return 0;
	}

	C c;
	if ( matchCase )
	{
		for( unsigned int i=0; i<m_len; i++ )
		{
			for( unsigned int j=0; (c = buf[j]) != 0; j++ )
			{
				if ( c != m_str[i+j] )
				{
					break;
				}
			}

			if ( !c )
			{
				return m_str + i;
			}
		}
	}
	else
	{
		for( unsigned int i=0; i<m_len; i++ )
		{
			for( unsigned int j=0; (c = tolower(buf[j])) != 0; j++ )
			{
				if ( c != (C)tolower(m_str[i+j]) )
				{
					break;
				}
			}

			if ( !c )
			{
				return m_str + i;
			}
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::insert( const C* buf, const unsigned int len /*=-1*/, const unsigned int startPos /*=0*/ )
{
	// compute the len if it wasn't fed in (can't use strlen() since this is sizeof <C>)
	unsigned int realLen = len;
	if ( realLen == (unsigned int)-1 )
	{
		if ( !buf )
		{
			return *this; // insert 0? done
		}
		
		for( realLen = 0; buf[realLen]; realLen++ );
	}

	if ( realLen == 0 ) // insert 0? done
	{
		return *this;
	}

	alloc( m_len + realLen, true ); // make sure there is enough room for the new string
	if ( startPos >= m_len )
	{
		if ( buf )
		{
			memcpy( m_str + m_len, buf, realLen * sizeof(C) );
		}
	}
	else
	{
		if ( startPos != m_len )
		{
			memmove( m_str + realLen + startPos, m_str + startPos, m_len * sizeof(C) );
		}
		
		if ( buf )
		{
			memcpy( m_str + startPos, buf, realLen * sizeof(C) );
		}
	}

	m_len += realLen;
	m_str[m_len] = 0;

	return *this;
}

//-----------------------------------------------------------------------------
template<class C> CTstr<C>& CTstr<C>::append( const C c )
{
	if ( m_len >= m_buflen )
	{
		alloc( ((m_len * 3) / 2) + 1, true ); // single-character, expect a lot more are coming so alloc some buffer space
	}
	m_str[ m_len++ ] = c;
	m_str[ m_len ] = 0;
	return *this;
}

}
#endif
