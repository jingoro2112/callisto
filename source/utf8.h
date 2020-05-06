#ifndef UTF8_H
#define UTF8_H
/*------------------------------------------------------------------------------*/

#include "str.h"

//------------------------------------------------------------------------------
inline int unicodeToUTF8( const wchar_t* unicode, const int length, Cstr& out )
{
	size_t len = length ? length : wcslen(unicode);

	for( size_t i=0; i<len; ++i )
	{
		if ( unicode[i] <= 0x7F ) 
		{
			out += (char)unicode[i];
		}
		else if (unicode[i] <= 0x7FF) 
		{
			out += (char)(0xC0 | (0x1F & (unicode[i] >> 6)));
			out += (char)(0x80 | (0x3F & unicode[i]));
		} 
		else if (unicode[i] <= 0xFFFF) 
		{
			out += (char)(0xE0 | (0x0F & (unicode[i] >> 12)));
			out += (char)(0x80 | (0x3F & (unicode[i] >> 6)));
			out += (char)(0x80 | (0x3F & unicode[i]));
		}
		else if (unicode[i] <= 0x10FFFF) 
		{
#if (WCHAR_MAX > 0xFFFF)
			out += (char)(0xF0 | (0x07 & (unicode[i] >> 18)));
			out += (char)(0x80 | (0x3F & (unicode[i] >> 12)));
			out += (char)(0x80 | (0x3F & (unicode[i] >> 6)));
			out += (char)(0x80 | (0x3F & unicode[i]));
#else
			out += "#";
#endif
		}
	}

	return out.size();
}

//------------------------------------------------------------------------------
inline unsigned int UTF8ToUnicode( const unsigned char* in, Wstr& str, const int charsToRead =0 )
{
	int chars = 0;
	unsigned int pos = 0;
	for( ; in[pos]; ++pos )
	{
		if ( charsToRead && (++chars > charsToRead) )
		{
			break;
		}

		if ( !(in[pos] & 0x80) ) // simple case, lower 127?
		{
			str.append( in[pos] );
			continue;
		}

		// nope. turn on the thrusters
		
		unsigned int unicodeCharacter = 0;
			
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
							pos += 5;
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

							pos += 4;
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

						pos += 3;
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
					
					pos += 2;
				}
			}
			// 0x110xxxxx:
			else if ( in[pos+1] && ((in[pos+1] & 0xC0) == 0x80) )
			{
				// 0x110xxxxx : 0x10000000
				unicodeCharacter = (((unsigned int)in[pos]) & 0x1F) << 6
								   | (((unsigned int)in[pos+1]) & 0x7F);

				++pos;
			}
		}

		str.append( (wchar_t)unicodeCharacter );
	}

	return pos;
}

#endif
