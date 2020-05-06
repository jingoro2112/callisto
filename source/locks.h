#ifndef LOCKS_HPP
#define LOCKS_HPP
/*------------------------------------------------------------------------------*/

#include "arch.h"

namespace Locks
{

#ifdef _WIN32

//------------------------------------------------------------------------------
class Spinlock
{
public:
	Spinlock() { m_lock = 0; }

	void lock() { while ( InterlockedCompareExchange(&m_lock, 1, 0) != 0 ) Sleep(0); }
	bool trylock() { return InterlockedCompareExchange(&m_lock, 1, 0) == 0; }
	void unlock() { m_lock = 0; }

private:
	volatile long m_lock;
};

#elif __MACH__

//-----------------------------------------------------------------------
class Spinlock
{
public:

	Spinlock() { __asm__ __volatile__ ("" ::: "memory"); m_lock=0; }

	void lock() { while( !__sync_bool_compare_and_swap(&m_lock, 0, 1) ) sched_yield(); }
	bool trylock() { return __sync_bool_compare_and_swap(&m_lock, 0, 1) == 1; }
	void unlock() { __asm__ __volatile__ ("" ::: "memory");	m_lock=0; }

private:
	int m_lock;
};

#else

//-----------------------------------------------------------------------
class Spinlock
{
public:

	Spinlock() { pthread_spin_init( &m_lock, 0 ); }
	~Spinlock() { pthread_spin_destroy( &m_lock ); }
	
	void lock() { pthread_spin_lock( &m_lock ); }
	bool trylock() { return !pthread_spin_trylock(&m_lock); }
	void unlock() { pthread_spin_unlock( &m_lock ); }
		
private:
	pthread_spinlock_t m_lock;
};

#endif

#ifdef _WIN32
//------------------------------------------------------------------------------
class Lock
{
public:
	Lock() { InitializeCriticalSection(&m_lock); }
	~Lock() { DeleteCriticalSection(&m_lock); }

	void lock() { EnterCriticalSection(&m_lock); }
	bool trylock() { return TryEnterCriticalSection( &m_lock ) ? true : false; }
	void unlock() { LeaveCriticalSection(&m_lock); }

private:
	CRITICAL_SECTION m_lock;
};

#else

//-----------------------------------------------------------------------
class Lock
{
public:
	Lock() { pthread_mutex_init(&m_mutex, 0);}
	~Lock() { pthread_mutex_destroy(&m_mutex); }

	void lock() { pthread_mutex_lock(&m_mutex); }
	bool trylock() { return !pthread_mutex_trylock(&m_mutex); }
	void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
	pthread_mutex_t m_mutex;
};

#endif

//------------------------------------------------------------------------------
class SpinlockScoped
{
public:
	SpinlockScoped( Spinlock& lock ) : m_lock(&lock), m_held(true) { lock.lock(); }
	SpinlockScoped() : m_lock(0), m_held(false) {}
	~SpinlockScoped() { unlock(); }
	
	void unlock() { if ( m_held && m_lock ) { m_lock->unlock(); m_held = false; } }
	void lock() { if ( !m_held && m_lock ) { m_lock->lock(); m_held = true; } }
	void set( Spinlock& lock, bool held ) { unlock(); m_lock = &lock; m_held = held; }
																	
private:
	Spinlock* m_lock;
	bool m_held;
};

//------------------------------------------------------------------------------
class LockScoped
{
public:
	LockScoped( Lock& lock ) : m_lock(&lock), m_held(true) { lock.lock(); }
	LockScoped() : m_lock(0), m_held(false) {}
	~LockScoped() { unlock(); }
	
	void unlock() { if ( m_held && m_lock ) { m_lock->unlock(); m_held = false; } }
	void lock() { if ( !m_held && m_lock ) { m_lock->lock(); m_held = true; } }
	void set( Lock& lock, bool held ) { unlock(); m_lock = &lock; m_held = held; }
	
private:
	Lock* m_lock;
	bool m_held;
};

//------------------------------------------------------------------------------
// very fast read/write lock. Write-locks are favored, this
// class spins on a sleep(0) to acquire, NOT an OS event, so write lock is meant
// to be held for a very short time
class RWSpinlock
{
public:
	RWSpinlock()
	{
		m_waiterCount = 0;
		m_accessCount = 0;
	}

	int convertToRead( bool yieldToWriteWaiters =false )
	{
		if ( m_waiterCount && yieldToWriteWaiters )
		{
			m_accessCount = 0;
			return readLock();
		}

		m_accessCount = 1;
		return 0;
	}

	// if multiple threads waiting on write there is no guarantee this one
	// will get it.
	int convertToWrite()
	{
		unlock();
		return writeLock();
	}

	// if a writer is trying to get the lock, allow it and re-acquire,
	// otherwise noop. This method ASSUMES THE READ LOCK IS HELD
	bool yieldToWrite()
	{
		if ( !m_waiterCount ) // no one wants it
		{
			return false;
		}

		// otherwise bounce lock, this will always allow the write
		// waiter(s) to take ownership
		unlock();
		readLock();
		return true;
	}

	// spin waiting for a read lock if any writers have (or are waiting for) a lock
	int readLock()
	{
		int spins = 0;
		m_lock.lock();

		while( m_waiterCount || (m_accessCount < 0) )
		{
			m_lock.unlock();

			while( m_waiterCount || (m_accessCount < 0) )
			{
				spins++;
#ifdef _WIN32
				Sleep(0);
#else
				usleep(0);
#endif
			}

			m_lock.lock();
		}

		InterlockedIncrement( &m_accessCount );
		m_lock.unlock();

		return spins;
	}

	// spin waiting for a write lock until all readers have released
	int writeLock()
	{
		int spins = 0;
		m_lock.lock();
		m_waiterCount++;

		while( m_accessCount )
		{
			m_lock.unlock();

			while( m_accessCount )
			{
				spins++;
#ifdef _WIN32
				Sleep(0);
#else
				usleep(0);
#endif
			}

			m_lock.lock();
		}

		m_waiterCount--;
		m_accessCount = -1;

		m_lock.unlock();
		return spins;
	}

	void unlock()
	{
		if ( m_accessCount > 0 ) // one of them MUST be us, don't care which
		{
			InterlockedDecrement( &m_accessCount );
		}
		else // otherwise it must be -1 by definition, set it to zero
		{
			m_accessCount = 0;
		}
	}

private:

	volatile long m_waiterCount;
	volatile long m_accessCount;
	Lock m_lock;
};

//------------------------------------------------------------------------------
class RWSpinlockReadScoped
{
public:
	RWSpinlockReadScoped( RWSpinlock& lock ) : m_lock(&lock), m_held(true) { lock.readLock(); }
	RWSpinlockReadScoped() : m_lock(0), m_held(false) {}
	~RWSpinlockReadScoped() { unlock(); }
	
	void unlock() { if ( m_held && m_lock ) { m_lock->unlock(); m_held = false; } }
	void lock() { if ( !m_held && m_lock ) { m_lock->readLock(); m_held = true; } }
	void set( RWSpinlock& lock, bool held ) { unlock(); m_lock = &lock; m_held = held; }
	
private:
	RWSpinlock* m_lock;
	bool m_held;
};

//------------------------------------------------------------------------------
class RWSpinlockWriteScoped
{
public:
	RWSpinlockWriteScoped( RWSpinlock& lock ) : m_lock(&lock), m_held(true) { lock.writeLock(); }
	RWSpinlockWriteScoped() : m_lock(0), m_held(false) {}
	~RWSpinlockWriteScoped() { unlock(); }
	
	void unlock() { if ( m_held && m_lock ) { m_lock->unlock(); m_held = false; } }
	void lock() { if ( !m_held && m_lock ) { m_lock->writeLock(); m_held = true; } }
	void set( RWSpinlock& lock, bool held ) { unlock(); m_lock = &lock; m_held = held; }
	
private:
	RWSpinlock* m_lock;
	bool m_held;
};

}

#endif
