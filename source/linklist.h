#ifndef LINKLIST_HPP
#define LINKLIST_HPP
/*------------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
template<class L> class CLinkList
{
public:

	struct Node
	{
		L item;
		Node *next;
		Node *prev;
	};

	inline CLinkList( void (*clear)( L& item ) =0 );
	
	~CLinkList() { clear(); }

	inline void clear();
	void resetIteration() { m_current = 0; }

	unsigned int count() const { return m_count; }

	inline void reverse();

	inline L* addHead();
	inline L* addTail();
	inline L* add() { return addTail(); }
	inline L* addBeforeCurrent();
	inline Node* addBeforeNode( Node &node )
	{
		Node* N = new Node;
		m_count++;
		N->next = &node;
		N->prev = node.prev;
		node.prev = N;
		if ( N->prev )
		{
			N->prev->next = N;
		}
		else
		{
			m_list = N;
		}
		
		return N;
	}
	
	inline L* addAfterCurrent();
	inline Node* addAfterNode( Node &node )
	{
		Node* N = new Node;
		m_count++;
		N->next = node.next;
		node.next = N;
		N->prev = &node;
		if ( N->next )
		{
			N->next->prev = N;
		}
		else
		{
			m_tail = N;
		}

		return N;
	}

	inline void unlinkNode( Node* n, bool release =true );
	Node* getUnlinkedNode() { return new Node; }
	void releaseNode( Node *n ) { delete n; }

	inline bool remove( L* item );
	inline bool removeCurrent();

	bool popFirst( L* copyTo =0 ) { if ( ((m_current = m_list) != 0 ) && copyTo ) { *copyTo = m_current->item; } return removeCurrent(); }
	bool popHead( L* copyTo =0 ) { if ( ((m_current = m_list) != 0 ) && copyTo ) { *copyTo = m_current->item; } return removeCurrent(); }
	bool popLast( L* copyTo =0 ) { if ( ((m_current = m_tail) != 0 ) && copyTo ) { *copyTo = m_current->item; } return removeCurrent(); }
	bool popTail( L* copyTo =0 ) { if ( ((m_current = m_tail) != 0 ) && copyTo ) { *copyTo = m_current->item; } return removeCurrent(); }

	L popFirstItem() { L ret = getFirstItem(); removeCurrent(); return ret; }
	L popHeadItem() { L ret = getHeadItem(); removeCurrent(); return ret; }
	L popLastItem() { L ret = getLastItem(); removeCurrent(); return ret; }
	L popTailItem() { L ret = getTailItem(); removeCurrent(); return ret; }

	Node* popHeadNode() { m_current = m_list; return popCurrentNode(); }
	Node* popTailNode() { m_current = m_tail; return popCurrentNode(); }
	Node* popFirstNode() { m_current = m_list; return popCurrentNode(); }
	Node* popLastNode() { m_current = m_tail; return popCurrentNode(); }
	Node* popCurrentNode() { Node* ret = m_current; unlinkNode(m_current, false); return ret; }

	L* getHead() { m_current = m_list; return getCurrent(); }
	L* getTail() { m_current = m_tail; return getCurrent(); }
	L* getFirst() { m_current = m_list; return getCurrent(); }
	L* getLast() { m_current = m_tail; return getCurrent(); }
	
	L getHeadItem() { m_current = m_list; return getCurrentItem(); }
	L getTailItem() { m_current = m_tail; return getCurrentItem(); }
	L getFirstItem() { m_current = m_list; return getCurrentItem(); }
	L getLastItem() { m_current = m_tail; return getCurrentItem(); }
	
	Node* getHeadNode() { m_current = m_list; return m_current; }
	Node* getTailNode() { m_current = m_tail; return m_current; }
	Node* getFirstNode() { m_current = m_list; return m_current; }
	Node* getLastNode() { m_current = m_tail; return m_current; }

	L* getNext() { m_current = m_current ? m_current->next : 0; return getCurrent(); }
	L getNextItem() { m_current = m_current ? m_current->next : 0; return getCurrentItem(); }
	Node* getNextNode() { m_current = m_current ? m_current->next : 0; return m_current; }

	L* getPrev() { m_current = m_current ? m_current->prev : m_list; return getCurrent(); }
	L getPrevItem() { m_current = m_current ? m_current->prev : m_list; return getCurrentItem(); }
	Node* getPrevNode() { m_current = m_current ? m_current->prev : m_list; return m_current; }

	L* getCurrent() const { return m_current ? &m_current->item : 0; }
	L getCurrentItem() const { return m_current ? m_current->item : 0; }
	Node* getCurrentNode() const { return m_current; }

	L* operator[]( const int index );

	void sort( int (*compareFunc)(const L& item1, const L& item2) );

	// copy and assignment are both groovy
	inline CLinkList<L>( const CLinkList<L> &other );
	inline CLinkList<L>& operator= (const CLinkList<L>& other );

public:
	class Iterator
	{
	public:
		Iterator( const CLinkList<L> &list ) { iterate(list); }
		Iterator() : m_list(0), m_current(0) {}
		bool removeCurrent();
		L* getCurrent() { return m_current ? &m_current->item : 0; }
		L* getFirst() { m_current = m_list ? m_list->m_list : 0; return m_current ? &m_current->item : 0; }
		L* getNext() { m_current = m_current ? m_current->next : 0; return m_current ? &m_current->item : 0; }
		L* getPrev() { m_current = m_current ? m_current->prev : 0; return m_current ? m_current->item : 0; }
		L getCurrentItem() { return m_current ? m_current->item : 0; }
		L getFirstItem() { m_current = m_list ? m_list->m_list : 0; return m_current ? m_current->item : 0; }
		L getNextItem() { m_current = m_current ? m_current->next : 0; return m_current ? m_current->item : 0; }

		bool operator!=( const Iterator& other ) { return m_current != other.m_current; }
		L& operator* () const { return m_current->item; }
		const Iterator operator++() { if ( m_current ) m_current = m_current->next; return *this; }

		void iterate( CLinkList<L> const& list ) { m_list = &list; m_current = m_list->m_list; }

		Iterator begin() const { return m_list ? Iterator(*m_list) : Iterator(); }
		Iterator end() const { return Iterator(); }
		unsigned int count() const { return m_list ? m_list->count() : 0; }

	private:
		CLinkList<L> const* m_list;
		Node *m_current;
	};

	Iterator begin() const { return Iterator(*this); }
	Iterator end() const { return Iterator(); }

private:

	Node *m_list;
	Node *m_tail;
	Node *m_current;
	unsigned int m_count;
	void (*m_clear)( L& item );
};

//-----------------------------------------------------------------------------
template<class L> CLinkList<L>::CLinkList( void (*clear)( L& item ) )
{
	m_list = 0;
	m_tail = 0;
	m_current = 0;
	m_count = 0;
	m_clear = clear;
}

//-----------------------------------------------------------------------------
template<class L> void CLinkList<L>::clear()
{
	while( m_list )
	{
		Node* next = m_list->next;
		if ( m_clear )
		{
			m_clear( m_list->item );
		}

		delete m_list;

		m_list = next;
	}

	m_tail = 0;
	m_current = 0;
	m_count = 0;
}

//-----------------------------------------------------------------------------
template<class L> void CLinkList<L>::reverse()
{
	Node* head = 0;

	m_tail = m_list;

	while( m_list )
	{
		Node* next = m_list->next;
		m_list->next = head;
		if ( head )
		{
			head->prev = m_list;
		}
		head = m_list;
		head->prev = 0;
		m_list = next;
	}

	m_list = head;
}

//-----------------------------------------------------------------------------
template<class L> L* CLinkList<L>::addHead()
{
	Node* N = new Node;

	m_count++;

	N->next = m_list;
	if ( m_list )
	{
		m_list->prev = N;
	}
	N->prev = 0;
	m_list = N;

	if ( !m_tail )
	{
		m_tail = m_list;
	}

	return &(N->item);
}

//-----------------------------------------------------------------------------
template<class L> L* CLinkList<L>::addTail()
{
	Node* N = new Node;

	m_count++;

	N->next = 0;

	if ( m_tail )
	{
		N->prev = m_tail;
		m_tail->next = N;
	}
	else
	{
		N->prev = 0;
		m_list = N;
	}

	m_tail = N;

	return &(N->item);
}

//-----------------------------------------------------------------------------
template<class L> L* CLinkList<L>::addBeforeCurrent()
{
	if ( !m_current )
	{
		return 0;
	}

	Node* N = new Node;

	m_count++;

	N->next = m_current;
	N->prev = m_current->prev;
	m_current->prev = N;
	if ( N->prev )
	{
		N->prev->next = N;
	}
	else
	{
		m_list = N;
	}

	return &(N->item);
}

//-----------------------------------------------------------------------------
template<class L> L* CLinkList<L>::addAfterCurrent()
{
	if ( !m_current )
	{
		return 0;
	}

	Node* N = new Node;

	m_count++;

	N->next = m_current->next;
	m_current->next = N;
	N->prev = m_current;
	if ( N->next )
	{
		N->next->prev = N;
	}
	else
	{
		m_tail = N;
	}

	return &(N->item);
}

//-----------------------------------------------------------------------------
template<class L> bool CLinkList<L>::removeCurrent()
{
	if ( !m_current )
	{
		return false;
	}

	m_count--;

	if ( m_clear )
	{
		m_clear( m_current->item );
	}

	if ( m_current == m_list )
	{
		if ( m_current == m_tail )
		{
			m_tail = 0;
		}

		if ( (m_list = m_current->next) )
		{
			m_list->prev = 0;
		}

		delete m_current;
		m_current = 0;	
	}
	else
	{
		if ( m_current == m_tail )
		{
			m_tail = m_current->prev;
		}

		Node *prev = m_current->prev;
		
		if ( (m_current->prev->next = m_current->next) )
		{
			m_current->next->prev = prev;
		}

		delete m_current;
		m_current = prev;
	}

	return true;
}

//-----------------------------------------------------------------------------
template<class L> void CLinkList<L>::unlinkNode( Node *n, bool release )
{
	if ( !n )
	{
		return;
	}

	m_count--;
	
	if ( n == m_list )
	{
		if ( n->next )
		{
			n->next->prev = 0;
			m_list = n->next;
		}
		else
		{
			m_list = 0;
			m_tail = 0;
		}

		if ( m_current == n )
		{
			m_current = 0;	
		}
	}
	else
	{
		Node *prev = n->prev;
		n->prev->next = n->next;
		if ( n->next )
		{
			n->next->prev = prev;
		}
		else
		{
			m_tail = prev;
		}

		if ( m_current == n )
		{
			m_current = prev;
		}
	}

	if ( release )
	{
		delete n;
	}
}

//-----------------------------------------------------------------------------
template<class L> bool CLinkList<L>::remove( L* item )
{
	if ( !item )
	{
		return false;
	}

	for ( Node *node = m_list ; node ; node = node->next )
	{
		if ( &(node->item) == item )
		{
			m_count--;

			if ( node == m_list )
			{
				if ( node->next )
				{
					node->next->prev = 0;
					m_list = node->next;
				}
				else
				{
					m_list = 0;
					m_tail = 0;
				}
			}
			else
			{
				Node *prev = node->prev;
				node->prev->next = node->next;
				if ( node->next )
				{
					node->next->prev = prev;
				}
				else
				{
					m_tail = prev;
				}
			}

			if ( m_clear )
			{
				m_clear( node->item );
			}

			delete node;
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
template<class L> L* CLinkList<L>::operator[]( const int index )
{
	int count = 0;
	for ( Node *node = m_list ; node ; node = node->next, count++ )
	{
		if ( count == index )
		{
			return &node->item;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
template<class L> void CLinkList<L>::sort( int (*compareFunc)(const L& item1, const L& item2) )
{
	if ( m_count < 2 || !compareFunc )
	{
		return;
	}

	m_current = 0; // invalidate current

	Node *sortedList = m_list;
	m_list = m_list->next;
	sortedList->next = 0;
	sortedList->prev = 0;
	m_tail = sortedList;

	while( m_list ) // consume the list from the head
	{
		Node* next = m_list->next;

		Node* E = sortedList;
		for( ; E; E=E->next )
		{
			if ( compareFunc(m_list->item, E->item) > 0 ) // do I come before the current entry?
			{
				// yes! link in
				if ( (m_list->prev = E->prev) )
				{
					m_list->prev->next = m_list;
				}
				else // no previous, we should become the head
				{
					sortedList = m_list;
				}

				m_list->next = E;
				E->prev = m_list;

				goto foundOne;
			}
		}

		// fell out of loop, link to the tail
		m_tail->next = m_list;
		m_list->next = 0;
		m_list->prev = m_tail;

		m_tail = m_list; // new tail

foundOne:

		m_list = next;
	}

	m_list = sortedList; // now the list has been re-linked, re attach it to the member pointer
}

//------------------------------------------------------------------------------
template<class L> CLinkList<L>::CLinkList( const CLinkList<L>& other )
{
	m_list = 0;
	m_tail = 0;
	m_current = 0;
	m_count = 0;
	m_clear = other.m_clear;

	*this = other;
}

//------------------------------------------------------------------------------
template<class L> CLinkList<L>& CLinkList<L>::operator=( const CLinkList<L>& other )
{
	if ( this != &other )
	{
		clear();
		Iterator iter(other);
		for( L* item = iter.getFirst(); item; item = iter.getNext() )
		{
			*addTail() = *item;
		}
		
		m_clear = other.m_clear;
	}
	
	return *this;
}

//------------------------------------------------------------------------------
template<class L> bool CLinkList<L>::Iterator::removeCurrent()
{
	if ( !m_current )
	{
		return false;
	}
	
	Node* newEntry = m_current->prev; // make sure the list stays sane after the removal
	(const_cast<CLinkList<L>*>(m_list))->remove( &m_current->item );
	m_current = newEntry;
	return true;
}

#endif
