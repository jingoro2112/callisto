#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP
/*------------------------------------------------------------------------------*/

#include <memory.h>
#include "object_tpool.h"

//-----------------------------------------------------------------------------
template <typename H> class CHashTable
{
public:

	inline CHashTable( void (*clear)(H& item) =0 );

	void setClearFunction( void (*clear)(H& item) ) { m_clear = clear; }
	
	inline H* add( const unsigned int key );
	inline H addItem( H item, const unsigned int key );

	inline bool remove( const unsigned int key );
	inline void clear();

	void resetBucketCount()
	{
		clear();
		m_mod = 4;
		delete[] m_list;
		m_list = new Node*[ m_mod ];
		memset( m_list, 0, m_mod * sizeof(Node*) );
	}
	
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

	struct Node
	{
		H item;
		unsigned int key;
		Node *next; // next in the hash link chain
	};

	// it can iterate, but not super-efficiently
	class Iterator
	{
	public:
		Iterator( CHashTable<H> const& list ) { m_list = &list; }
		unsigned int getCurrentKey() { return m_current ? m_current->key : 0; }

		H* getFirst()
		{
			m_current = 0;
			m_onMod = 0;
			for( m_onMod = 0; m_onMod < m_list->m_mod; ++m_onMod )
			{
				if ( (m_current = m_list->m_list[m_onMod]) )
				{
					return &m_current->item;
				}
			}
			
			return 0;
		}

		H getFirstItem()
		{
			m_current = 0;
			m_onMod = 0;
			for( m_onMod = 0; m_onMod < m_list->m_mod; ++m_onMod )
			{
				if ( (m_current = m_list->m_list[m_onMod]) )
				{
					return m_current->item;
				}
			}

			return 0;
		}

		H* getNext()
		{
			if ( m_current && m_current->next )
			{
				m_current = m_current->next;
				return &(m_current->item);
			}
			
			for( ++m_onMod; m_onMod < m_list->m_mod; ++m_onMod )
			{
				if ( (m_current = m_list->m_list[m_onMod]) )
				{
					return &(m_current->item);
				}
			}

			return 0;
		}

		H getNextItem()
		{
			if ( m_current && m_current->next )
			{
				m_current = m_current->next;
				return m_current->item;
			}

			for( ++m_onMod; m_onMod < m_list->m_mod; ++m_onMod )
			{
				if ( (m_current = m_list->m_list[m_onMod]) )
				{
					return m_current->item;
				}
			}

			return 0;
		}

		// returns the next item in line
		H* removeCurrent()
		{
			if ( !m_current )
			{
				return 0;
			}

			unsigned int key = m_current->key;
			H* ret = getNext();
			m_list->remove( key );
			return ret;
		}

	private:
		unsigned int m_onMod;
		CHashTable<H> const* m_list;
		Node *m_current;
	};
	
private:

	static CObjectTPool<Node> m_hashNodes;

	// neither of these are defined
	CHashTable<H>( const CHashTable<H> &other );
	CHashTable<H>& operator=( const CHashTable<H>& other );
		
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
	setClearFunction( clear );
	resize( 4 );
}

//-----------------------------------------------------------------------------
template<class H> H* CHashTable<H>::add( const unsigned int key )
{
	Node *N = m_hashNodes.get();
	N->key = key;

	const int p = (int)(key % m_mod);
	N->next = m_list[p];
	m_list[p] = N;

	if ( ++m_count > m_loadThreshold )
	{
		resize( m_mod * 2 );
	}

	return &N->item;
}

//-----------------------------------------------------------------------------
template<class H> H CHashTable<H>::addItem( H item, const unsigned int key )
{
	return (*add(key) = item);
}

//-----------------------------------------------------------------------------
template<class H> bool CHashTable<H>::remove( unsigned int key )
{
	// find the position
	const int p = (int)(key % m_mod);

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
			
			m_hashNodes.release( node );

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
template<class H> void CHashTable<H>::clear()
{
	for( unsigned int l=0; l<m_mod ; l++ )
	{
		while( m_list[l] )
		{
			Node *N = m_list[l]->next;
			
			if ( m_clear )
			{
				m_clear( m_list[l]->item );
			}
			m_hashNodes.release( m_list[l] );
			m_list[l] = N;
		}
	}

	m_count = 0;
}

#endif
