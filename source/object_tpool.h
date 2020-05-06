#ifndef OBJECT_TPOOL_H
#define OBJECT_TPOOL_H
/*------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
template<class T> class CObjectTPool
{
public:

	inline CObjectTPool( const int maxFreeSize =32, void (*clear)( T& item ) =0  );
	inline ~CObjectTPool()
	{
		// free has had clear called on it already, so just clear the list
		while( m_freeList )
		{
			Node* next = m_freeList->CObjectTPool_Node_next;
			delete m_freeList;
			m_freeList = next;
		}

		m_maxFreeSize = 0;
		m_freeSize = 0;
	}

	// gets a new member of the pool
	T* get()
	{
		T* ret;
		if ( (ret = m_freeList) )
		{
			// alloc from free list
			--m_freeSize;
			m_freeList = m_freeList->CObjectTPool_Node_next;
		}
		else
		{
			// create a new object to pool
			ret = new Node;
			((Node*)ret)->CObjectTPool_refCount = 1;
		}
		return ret;
	}

	// adds a refcount to the member for release() to track
	inline void getReference( T* object );

	inline void release( T* object );

	unsigned int getFreeListCount() const { return m_freeSize; }
	unsigned int getMaxFreeSize() const { return m_maxFreeSize; }
	
private:

	// by using inheritance the pointer value can be directly used
	// without needing to find the node that contains it
	struct Node : T
	{
		// choose names unlikely to collide (a downside to this approach)
#pragma pack(4)
		int CObjectTPool_refCount;
		Node* CObjectTPool_Node_next;
#pragma pack()
	};

	Node* m_freeList; // pool
	unsigned int m_maxFreeSize;
	unsigned int m_freeSize;
	void (*m_clear)( T& item );
};

//------------------------------------------------------------------------------
template<class T> CObjectTPool<T>::CObjectTPool( const int maxFreeSize /*=32*/, void (*clear)( T& item ) /*=0*/ )
{
	m_freeList = 0;
	m_maxFreeSize = maxFreeSize;
	m_freeSize = 0;
	m_clear = clear;
}

//------------------------------------------------------------------------------
template<class T> void CObjectTPool<T>::getReference( T* node )
{
	++((Node*)node)->CObjectTPool_refCount;
}

//------------------------------------------------------------------------------
template<class T> void CObjectTPool<T>::release( T* node )
{
	if ( !node || --((Node*)node)->CObjectTPool_refCount )
	{
		return;
	}

	if ( m_clear )
	{
		m_clear( *node );
	}

	if( m_freeSize >= m_maxFreeSize ) // but exceeding desired free size?
	{
		delete node; // just murder it
	}
	else
	{
		// it is now removed from the allocated list, add to the free list
	
		++m_freeSize;
		++((Node*)node)->CObjectTPool_refCount;
		((Node*)node)->CObjectTPool_Node_next = m_freeList;
		m_freeList = (Node*)node;
	}
}

#endif
