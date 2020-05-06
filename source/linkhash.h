#ifndef LINKHASH_H
#define LINKHASH_H
/*------------------------------------------------------------------------------*/

#include <memory.h>
#include "object_tpool.h"

//-----------------------------------------------------------------------------
template <class H> class CLinkHash
{
public:

	inline CLinkHash( void (*clear)( H& item ) =0 );

	void setClearFunction( void (*clear)(H& item) ) { m_clear = clear; }

	inline H* add( const unsigned int key, bool addToTail =false );
	H addItem( H item, const unsigned int key, bool addToTail =false );
	inline bool remove( const unsigned int key );
	inline void removeCurrent() { if ( m_current ) remove( m_current->key ); }
	inline void clear( bool resetBucketCount =false );
	unsigned int count() const { return m_count; }
	inline void resize( unsigned int newBucketCount );
	H* getFirst() { m_current = m_head; return m_current ? &(m_current->item) : 0; }
	H getFirstItem() { m_current = m_head; return m_current ? m_current->item : 0; }
	H* getNext()
	{
		m_current = m_current ? m_current->nextIter :  m_head;
		return m_current ? &(m_current->item) : 0;
	}
	H getNextItem()
	{
		m_current = m_current ? m_current->nextIter : m_head;
		return m_current ? &(m_current->item) : 0;
	}
	
	H* getCurrent() { return m_current ? &(m_current->item) : 0; }
	H getCurrentItem() { return m_current ? m_current->item : 0; }
	unsigned int getCurrentKey() { return m_current ? m_current->key : 0; }

	inline void reverseIterationOrder();
	inline void firstLinkToLast();

	// to sort the iterate order
	inline void sort( int (*compareFunc)(const H& item1, const H& item2) );

	// copy and assignment are both groovy
	inline CLinkHash<H>( const CLinkHash<H> &other );
	inline CLinkHash<H>& operator=( const CLinkHash<H>& other );

	void resetIteration() { m_current = 0; }
	
	H* get( const unsigned int key, H addThisIfNotFound )
	{
		for( const Node *N = m_list[key % m_mod]; N ; N=N->next )
		{
			if ( N->key == key )
			{
				return (H *)&(N->item);
			}
		}

		H* item = add( key );
		*item = addThisIfNotFound;
		return item;
	}

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

	~CLinkHash() { clear(); delete[] m_list; }

	struct Node
	{
		H item;
		unsigned int key;
		Node *next; // next in the hash link chain
		Node *nextIter; // doubly-linked list for quick iteration/removal
		Node *prevIter;
	};

	class Iterator
	{
	public:
		Iterator( CLinkHash<H> const& list ) { iterate(list); }
		Iterator() : m_list(0), m_current(0) {}
		void reset() { m_current = 0; }
		void removeCurrent() { if ( m_current ) { Node* T = m_current->prevIter; (const_cast<CLinkHash<H>*>(m_list))->remove( m_current->key ); m_current = T; } }
		H* getCurrent() { return m_current ? &(m_current->item) : 0; }
		H* getFirst() { m_current = m_list ? m_list->m_head : 0; return m_current ? &(m_current->item) : 0; }
		H* getNext()
		{
			m_current = m_current ? m_current->nextIter : m_list->m_head;
			return m_current ? &(m_current->item) : 0;
		}
		H getCurrentItem() { return m_current ? m_current->item : 0; }
		H getFirstItem() { m_current = m_list ? m_list->m_head : 0; return m_current ? m_current->item : 0; }
		H getNextItem()
		{
			m_current = m_current ? m_current->nextIter : m_list->m_head;
			return m_current ? m_current->item : 0;
		}

		bool operator!=( const Iterator& other ) { return m_current != other.m_current; }
		H& operator* () const { return m_current->item; }
		const Iterator operator++() { if ( m_current ) m_current = m_current->nextIter; return *this; }

		unsigned int getCurrentKey() { return m_current ? m_current->key : 0; }
		void iterate( CLinkHash<H> const& list ) { m_list = &list; m_current = 0; }

		Iterator begin() const { return m_list ? Iterator(*m_list) : Iterator(); }
		Iterator end() const { return Iterator(); }
		unsigned int count() const { return m_list ? m_list->count() : 0; }

	private:
		CLinkHash<H> const* m_list;
		Node *m_current;
	};

	Iterator begin() const { return Iterator(*this); }
	Iterator end() const { return Iterator(); }
		
private:

	static CObjectTPool<Node> m_linkNodes;

	inline void link( Node* N, bool addToTail );

	Node *m_head;
	Node *m_current;
	Node **m_list;
	unsigned int m_mod;
	unsigned int m_count;
	unsigned int m_loadThreshold;
	void (*m_clear)( H& item );
};

//-----------------------------------------------------------------------------
template<class H> CLinkHash<H>::CLinkHash( void (*clear)( H& item ) )
{
	m_head = 0;
	m_current = 0;
	m_list = 0;
	m_mod = 0; 
	m_count = 0;
	setClearFunction( clear );
	resize( 4 );
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::link( CLinkHash<H>::Node* N, bool addToTail )
{
	if ( !m_head )
	{
		// first add
		N->nextIter = 0;
		N->prevIter = 0;
		m_head = N;
	}
	else if ( !addToTail )
	{
		N->nextIter = m_head;
		m_head->prevIter = N;
		N->prevIter = 0;
		m_head = N;
	}
	else
	{
		// a tail pointer is not maintained, have to find it
		for( Node *tail = m_head ;; tail=tail->nextIter )
		{
			if ( !tail->nextIter ) // we are here
			{
				tail->nextIter = N;
				N->prevIter = tail;
				N->nextIter = 0;
				break;
			}
		}
	}

	if ( ++m_count > m_loadThreshold )
	{
		resize( m_mod * 2 );
	}
}

//-----------------------------------------------------------------------------
template<class H> H* CLinkHash<H>::add( const unsigned int key, bool addToTail /*=false*/ )
{
	Node *N = m_linkNodes.get();
	
	N->key = key;

	const int p = (int)(key % m_mod);
	N->next = m_list[p];
	m_list[p] = N;

	link( N, addToTail );

	return &N->item;
}

//-----------------------------------------------------------------------------
template<class H> H CLinkHash<H>::addItem( H item, const unsigned int key, bool addToTail /*=false*/ )
{
	Node *N = m_linkNodes.get();

	N->item = item;
	N->key = key;

	const int p = (int)(key % m_mod);
	N->next = m_list[p];
	m_list[p] = N;

	link( N, addToTail );

	return N->item;
}

//-----------------------------------------------------------------------------
template<class H> bool CLinkHash<H>::remove( unsigned int key )
{
	// find the position
	const int p = (int)(key % m_mod);

	// iterate through, find/unlink the item
	Node *prev = 0;
	for( Node *node = m_list[p]; node ; node = node->next )
	{
		if ( node->key == key )
		{
			if ( node == m_current )
			{
				m_current = node->prevIter;
			}

			if ( prev )
			{
				prev->next = node->next;
			}
			else
			{
				m_list[p] = node->next;
			}

			if ( node == m_head )
			{
				if ( (m_head = node->nextIter) )
				{
					m_head->prevIter = 0;
				}
			}
			else
			{
				node->prevIter->nextIter = node->nextIter;

				if ( node->nextIter )
				{
					node->nextIter->prevIter = node->prevIter;
				}
			}

			if ( m_clear )
			{
				m_clear( node->item );
			}
			
			m_linkNodes.release( node );

			m_count--;
			return true;
		}

		prev = node;
	}

	return false;
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::resize( unsigned int newBucketCount )
{
	m_mod = newBucketCount < 4 ? 4 : newBucketCount;

	Node **oldList = m_list;
	m_list = new Node*[ m_mod ];
	memset( m_list, 0, m_mod * sizeof(Node*) );

	for( Node *N = m_head ; N ; N = N->nextIter )
	{
		int bucket = (int)(N->key % m_mod);
		N->next = m_list[bucket];
		m_list[bucket] = N;
	}

	m_loadThreshold = m_mod - (m_mod / 4);
	delete[] oldList;
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::clear( bool resetBucketCount )
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
			m_linkNodes.release( m_list[l] );
			m_list[l] = N;
		}
	}

	m_head = 0;
	m_current = 0;
	m_count = 0;

	if ( resetBucketCount )
	{
		m_mod = 4;
		delete[] m_list;
		m_list = new Node*[ m_mod ];
		memset( m_list, 0, m_mod * sizeof(Node*) );
	}
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::reverseIterationOrder()
{
	if ( m_head )
	{
		Node *prev = 0;
		for(;;)
		{
			Node *next = m_head->nextIter;
			m_head->nextIter = prev;
			if ( !next )
			{
				break;
			}
			prev = m_head;
			m_head = next;
		}
	}
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::firstLinkToLast()
{
	if ( !m_head || !m_head->nextIter )
	{   
		return;
	}

	Node *tail = m_head;
	m_head = m_head->nextIter;

	for( Node *N = m_head; N; N=N->nextIter )
	{   
		if ( !N->nextIter )
		{   
			N->nextIter = tail;
			tail->nextIter = 0;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
template<class H> void CLinkHash<H>::sort( int (*compareFunc)(const H& item1, const H& item2) )
{
	if ( m_count < 2 || !compareFunc )
	{
		return;
	}

	m_current = 0; // invalidate current

	Node *sortedList = m_head;
	m_head = m_head->nextIter;
	sortedList->nextIter = 0;
	sortedList->prevIter = 0;
	Node* tail = sortedList;

	while( m_head ) // consume the list from the head
	{
		Node* next = m_head->nextIter;

		Node* E = sortedList;
		for( ; E; E=E->nextIter )
		{
			if ( compareFunc(m_head->item, E->item) > 0 ) // do I come before the current entry?
			{
				// yes! link in
				if ( (m_head->prevIter = E->prevIter) )
				{
					m_head->prevIter->nextIter = m_head;
				}
				else // no previous, we should become the head
				{
					sortedList = m_head;
				}

				m_head->nextIter = E;
				E->prevIter = m_head;
				
				goto foundOne; // don't be a hater
			}
		}

		// fell out of loop, link to the tail
		tail->nextIter = m_head;
		m_head->nextIter = 0;
		m_head->prevIter = tail;

		tail = m_head; // new tail

foundOne:
		
		m_head = next;
	}

	m_head = sortedList; // now the list has been re-linked, re attach it to the member pointer
}

//------------------------------------------------------------------------------
template<class H> CLinkHash<H>::CLinkHash( const CLinkHash<H>& other )
{
	m_head = 0;
	m_current = 0;
	m_list = 0;
	m_mod = 0;
	m_count = 0;
	m_clear = other.m_clear;

	resize( 4 );

	*this = other;
}

//------------------------------------------------------------------------------
template<class H> CLinkHash<H>& CLinkHash<H>::operator=( const CLinkHash<H>& other )
{
	if ( this != &other )
	{
		clear( true );
		Iterator iter(other);
		for( H* item = iter.getFirst(); item; item = iter.getNext() )
		{
			addItem( *item, iter.getCurrentKey() );
		}
		
		reverseIterationOrder();
		
		m_clear = other.m_clear;
	}
	
	return *this;
}

#endif
