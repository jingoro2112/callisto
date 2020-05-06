#include "event.h"

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#endif

namespace Locks
{

#ifdef _WIN32

//------------------------------------------------------------------------------
Event::Event()
{
	m_broadcast = false;
	m_condition = CreateEvent( NULL, true, false, NULL );
}

//------------------------------------------------------------------------------
Event::~Event()
{
	CloseHandle( m_condition );
}

//------------------------------------------------------------------------------
void Event::reset()
{
	m_broadcast = false;
	ResetEvent( m_condition );
}

//------------------------------------------------------------------------------
void Event::signal()
{
	SetEvent( m_condition );
}

//------------------------------------------------------------------------------
void Event::broadcast()
{
	m_broadcast = true;
	SetEvent( m_condition );
}

//------------------------------------------------------------------------------
void Event::wait( bool reset /*=true*/ )
{
	while( WaitForSingleObject(m_condition, INFINITE) != WAIT_OBJECT_0 );

	if ( !m_broadcast && reset )
	{
		ResetEvent( m_condition );
	}
}

//------------------------------------------------------------------------------
bool Event::timedWait( unsigned int milliseconds, bool reset /*=true*/ )
{
	int ret = WaitForSingleObject( m_condition, milliseconds );

	if ( ret != WAIT_TIMEOUT )
	{
		if ( !m_broadcast && reset )
		{
			ResetEvent( m_condition );
		}

		return true;
	}

	return false;
}

#else

//------------------------------------------------------------------------------
Event::Event()
{
	m_flag = false;
	m_broadcast = false;
	pthread_cond_init( &m_condition, 0 );
	pthread_mutex_init( &m_mutex, 0 );
}

//------------------------------------------------------------------------------
Event::~Event()
{
	pthread_cond_destroy( &m_condition );
	pthread_mutex_destroy( &m_mutex );
}

//------------------------------------------------------------------------------
void Event::reset()
{
	pthread_mutex_lock( &m_mutex );
	m_flag = false;
	pthread_mutex_unlock( &m_mutex );
}

//------------------------------------------------------------------------------
void Event::signal()
{
	pthread_mutex_lock( &m_mutex );
	m_flag = true;
	pthread_cond_signal( &m_condition );
	pthread_mutex_unlock( &m_mutex );
}

//------------------------------------------------------------------------------
void Event::broadcast()
{
	pthread_mutex_lock( &m_mutex );
	m_broadcast = true;
	m_flag = true;
	pthread_cond_broadcast( &m_condition );
	pthread_mutex_unlock( &m_mutex );
}

//------------------------------------------------------------------------------
void Event::wait( bool reset /*=true*/ )
{
	pthread_mutex_lock( &m_mutex );
	while ( !m_flag )
	{
		int err;
		do
		{
			err = pthread_cond_wait( &m_condition, &m_mutex );

		} while ( err == EINTR || err == EAGAIN );
	}

	m_flag = !reset || m_broadcast;

	pthread_mutex_unlock( &m_mutex );
}

//------------------------------------------------------------------------------
bool Event::timedWait( unsigned int milliseconds, bool reset /*=true*/ )
{
	pthread_mutex_lock( &m_mutex );

	struct timespec abstime;

#ifdef __MACH__
	struct timeval tv;
	gettimeofday(&tv, NULL);
	abstime.tv_sec = tv.tv_sec + milliseconds / 1000;
	abstime.tv_nsec = (milliseconds % 1000) * 1000000;
#else
	clock_gettime( CLOCK_REALTIME, &abstime );
	abstime.tv_nsec += (milliseconds % 1000) * 1000000;
	abstime.tv_sec += milliseconds / 1000;
#endif

	if ( abstime.tv_nsec >= 1000000000 )
	{
		abstime.tv_nsec -= 1000000000;
		abstime.tv_sec += 1;
	}

	bool timeout = false;

	while( !m_flag && !timeout )
	{
		int err;
		do
		{
			err = pthread_cond_timedwait( &m_condition, &m_mutex, &abstime );

			if (
#ifdef ETIME
				  err == ETIME ||
#endif
				  err == ETIMEDOUT)
			{
				timeout = true;
			}

		} while ( !timeout && (err == EINTR || err == EAGAIN) );
	}

	if ( !timeout )
	{
		m_flag = !reset || m_broadcast;
	}

	pthread_mutex_unlock( &m_mutex );

	return !timeout;
}

#endif

}


