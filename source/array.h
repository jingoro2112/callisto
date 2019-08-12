#ifndef ARRAY_H
#define ARRAY_H
/*------------------------------------------------------------------------------*/

struct Value;

//-----------------------------------------------------------------------------
class Carray
{
public:

	Carray()
	{
		m_count = 0;
		m_size = 0;
		m_array = 0;
	}
	
	~Carray();

	unsigned int count() const { return m_count; }
	void remove( const unsigned int location, unsigned int count =1 );
	void alloc( const unsigned int size );
	Value& append();

	Value& operator[]( const unsigned int l );

private:

	Value** m_array;
	unsigned int m_size;
	unsigned int m_count;
};


#endif
