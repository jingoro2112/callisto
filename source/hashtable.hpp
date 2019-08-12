#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP
/*------------------------------------------------------------------------------*/

#include <memory.h>

//-----------------------------------------------------------------------------
template <class H> class CHashTable
{
public:

	inline CHashTable( void (*clear)( H& item ) =0 );

	inline H* add( const unsigned int key );
	void addItem( H item, const unsigned int key );
	inline bool remove( const unsigned int key );
	inline void clear( bool resetBucketCount =false );
	unsigned int count() const { return m_count; }
	inline void resize( unsigned int newBucketCount );

	H* get( const unsigned int key ) const
	{
		for( const Node *N = m_list[key % m_mod]; N ; N=N->next )
		{
			if ( N->key == key )
			{
				return (H *)&(N->item);
			}
		}

		return 0;
	}

	H getItem( const unsigned int key ) const
	{
		for( const Node *N = m_list[key % m_mod]; N ; N=N->next )
		{
			if ( N->key == key )
			{
				return N->item;
			}
		}
		return 0;
	}

	~CHashTable() { clear(); delete[] m_list; }

private:

	// neither of these are defined
	CHashTable<H>( const CHashTable<H> &other );
	CHashTable<H>& operator=( const CHashTable<H>& other );

	struct Node
	{
		H item;
		unsigned int key;
		Node *next; // next in the hash link chain
	};
		
	Node **m_list;
	unsigned int m_mod;
	unsigned int m_count;
	unsigned int m_loadThreshold;
	void (*m_clear)( H& item );
};

//-----------------------------------------------------------------------------
template<class H> CHashTable<H>::CHashTable( void (*clear)( H& item ) )
{
	m_list = 0;
	m_mod = 0; 
	m_count = 0;
	m_clear = clear;
	resize( 4 );
}

//-----------------------------------------------------------------------------
template<class H> H* CHashTable<H>::add( const unsigned int key )
{
	Node *N = new Node;
	
	N->key = key;

	const int p = (int)(key % m_mod);
	N->next = m_list[p];
	m_list[p] = N;

	m_count++;
	
	return &N->item;
}

//-----------------------------------------------------------------------------
template<class H> void CHashTable<H>::addItem( H item, const unsigned int key )
{
	Node *N = new Node;

	N->item = item;
	N->key = key;

	const int p = (int)(key % m_mod);
	N->next = m_list[p];
	m_list[p] = N;

	m_count++;
}

//-----------------------------------------------------------------------------
template<class H> bool CHashTable<H>::remove( unsigned int key )
{
	// find the position
	const int p = (int)(key % m_mod);

	// iterate through, find/unlink the item
	Node *prev = 0;
	for( Node *node = m_list[p]; node ; node = node->next )
	{
		if ( node->key == key )
		{
			if ( prev )
			{
				prev->next = node->next;
			}
			else
			{
				m_list[p] = node->next;
			}

			if ( m_clear )
			{
				m_clear( node->item );
			}
			
			delete node;

			m_count--;
			return true;
		}

		prev = node;
	}

	return false;
}

//-----------------------------------------------------------------------------
template<class H> void CHashTable<H>::resize( unsigned int newBucketCount )
{
	int oldMod = m_mod;
	
	m_mod = newBucketCount < 4 ? 4 : newBucketCount;

	Node **oldList = m_list;
	m_list = new Node*[ m_mod ];
	memset( m_list, 0, m_mod * sizeof(Node*) );

	for( int i=0; i<oldMod; i++ )
	{
		for( Node *N = oldList[i]; N;  )
		{
			Node *next = N->next;
			int bucket = (int)(N->key % m_mod);
			N->next = m_list[bucket];
			m_list[bucket] = N;
			N = next;
		}
	}

	m_loadThreshold = m_mod - (m_mod / 4);
	delete[] oldList;
}

//-----------------------------------------------------------------------------
template<class H> void CHashTable<H>::clear( bool resetBucketCount )
{
	if ( !m_list )
	{
		return;
	}
	
	for( unsigned int l=0; l<m_mod ; l++ )
	{
		while( m_list[l] )
		{
			Node *N = m_list[l]->next;

			if ( m_clear )
			{
				m_clear( m_list[l]->item );
			}
			delete m_list[l];
			m_list[l] = N;
		}
	}

	m_count = 0;

	if ( resetBucketCount )
	{
		m_mod = 4;
		delete[] m_list;
		m_list = new Node*[ m_mod ];
		memset( m_list, 0, m_mod * sizeof(Node*) );
	}
}

#endif
