#ifndef CALLISTO_ASCIIDUMP_H
#define CALLISTO_ASCIIDUMP_H
/*------------------------------------------------------------------------------*/

#include "c_str.h"

namespace Callisto
{

//------------------------------------------------------------------------------
inline const char* asciiDump( const void* d, unsigned int len, C_str* str =0 )
{
	const unsigned char* data = (char unsigned *)d;
	static C_str local;
	C_str *use = str ? str : &local;
	use->clear();
	for( unsigned int i=0; i<len; i++ )
	{
		use->appendFormat( "0x%08X: ", i );
		char dump[24];
		unsigned int j;
		for( j=0; j<16 && i<len; j++, i++ )
		{
			dump[j] = isgraph((unsigned char)data[i]) ? data[i] : '.';
			dump[j+1] = 0;
			use->appendFormat( "%02X ", (unsigned char)data[i] );
		}

		for( ; j<16; j++ )
		{
			use->appendFormat( "   " );
		}
		i--;
		*use += ": ";
		*use += dump;
		*use += "\n";
	}

	if ( !str )
	{
		printf( "%s\n", use->c_str() );
	}

	return use->c_str();
}

}

#endif
