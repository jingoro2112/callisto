#ifndef ARRAY_H
#define ARRAY_H
/*------------------------------------------------------------------------------*/

#include <memory.h>

//-----------------------------------------------------------------------------
template <class A> class Carray
{
public:

	Carray( void (*clear)(A& item) =0 )
	{
		m_count = 0;
		m_size = 0;
		m_array = 0;
		m_clear = clear;
	}

	void setClearFunction( void (*clear)(A& item) ) { m_clear = clear; }

	void clear()
	{
		if ( m_clear )
		{
			for( unsigned int l=0; l<m_size; ++l)
			{
				m_clear( m_array[l] );
			}
		}
		delete[] m_array;
		m_array = 0;
		m_count = 0;
		m_size = 0;
	}
	
	~Carray() { clear(); }

	unsigned int count() const { return m_count; }
	
	void remove( const unsigned int location, unsigned int count =1 )
	{
		unsigned int removeCount = 0;
		if ( m_clear )
		{
			for( unsigned int l = location; l<m_count && removeCount<count; ++l, ++removeCount )
			{
				m_clear( m_array[l] );
			}
		}

		memmove( m_array + location,
				 m_array + location + removeCount,
				 (m_count - (location + removeCount)) * sizeof(A) );

		m_count -= removeCount;
	}

	void alloc( const unsigned int size )
	{
		if ( size > m_size )
		{
			unsigned int newSize = ((size * 4) / 3) + 1;

			A* newArray = new A[newSize];
						
			memcpy( newArray, m_array, m_size * sizeof(A) );

			delete[] m_array;
			m_array = newArray;
			m_size = newSize;
		}
	}

	A& add( const unsigned int index =-1 )
	{
		if ( index == (unsigned int)-1 )
		{
			alloc( m_count + 1 );
			return m_array[ m_count++ ];
		}
		
		if ( index >= m_size )
		{
			alloc( index + 1 );
			m_count = index + 1;
		}
		else
		{
			alloc( m_count + 1 );
			memmove( m_array + index + 1, m_array + index, (m_count - index) * sizeof(A) );
			++m_count;
		}
		
		return m_array[index];
	}

	A& operator[]( const unsigned int l )
	{
		if ( l >= m_count )
		{
			alloc( l + 1 );
			m_count = l + 1;
		}

		return m_array[l];
	}

private:

	A* m_array;
	unsigned int m_size;
	unsigned int m_count;
	void (*m_clear)( A& item );
};

#endif
