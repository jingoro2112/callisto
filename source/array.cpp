#include "array.h"
#include "util.h"

//------------------------------------------------------------------------------
   Carray::~Carray()
{
	for( unsigned int l=0; l<m_size; ++l)
	{
		delete m_array[l];
	}
	delete[] m_array;
}

//------------------------------------------------------------------------------
void Carray::remove( const unsigned int location, unsigned int count )
{
	unsigned int removeCount = 0;
	for( unsigned int l = location; l<m_count && removeCount<count; ++l, ++removeCount )
	{
		delete m_array[l];
	}

	memmove( m_array + location,
			 m_array + location + removeCount,
			 (m_count - (location + removeCount)) * sizeof(Value*) );

	m_count -= removeCount;
}

//------------------------------------------------------------------------------
void Carray::alloc( const unsigned int size )
{
	if ( size > m_size )
	{
		unsigned int newSize = ((size * 4) / 3) + 1;

		Value** newArray = new Value*[newSize];

		unsigned int l = 0;
		for( ; l<m_count; l++ )
		{
			newArray[l] = m_array[l];
		}

		for( ; l<m_size; l++ )
		{
			newArray[l] = m_array[l];
		}

		for( ; l<newSize; l++ )
		{
			newArray[l] = new Value;
		}

		delete[] m_array;
		m_array = newArray;
		m_size = newSize;
	}
}

//------------------------------------------------------------------------------
Value& Carray::append()
{
	alloc( m_count+1 );
	return *( m_array[m_count++] );
}

//------------------------------------------------------------------------------
Value& Carray::operator[]( const unsigned int l )
{
	if ( l >= m_count )
	{
		alloc( l + 1 );
		m_count = l + 1;
	}

	return *(m_array[l]);
}

