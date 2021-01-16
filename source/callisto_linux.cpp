#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

#include "vm.h"

namespace Callisto
{

//------------------------------------------------------------------------------
void Callisto_getLine( C_str& line )
{
	line.clear();
	
	char* buffer = 0;
	size_t bufsize = 0;

	if ( getline(&buffer, &bufsize, stdin) != -1 )
	{
		line.set( buffer, bufsize );
		free( buffer );
	}
	else
	{
		line.clear();
	}
}

//------------------------------------------------------------------------------
void Callisto_sleep( const int milliseconds )
{
	usleep( milliseconds * 1000 );
}

//------------------------------------------------------------------------------
static void* callInThread_EX( void* arg )
{
	Callisto_ExecutionContext *exec = (Callisto_ExecutionContext *)arg;

	exec->function( exec );
	exec->state = Callisto_ThreadState::FromC | Callisto_ThreadState::Runnable;

	exec->callisto->OSWaitHandle.signal();

	return 0;
}

//------------------------------------------------------------------------------
int Callisto_startThread( Callisto_ExecutionContext* exec )
{
	pthread_attr_t attr;
	pthread_t tid;

	pthread_attr_init( &attr );

	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

	pthread_create( &tid, &attr, callInThread_EX, exec );

	pthread_attr_destroy( &attr );
	
	return CE_NoError;
}

//-----------------------------------------------------------------------
struct Callisto_WaitEvent_implementation
{
	pthread_cond_t m_condition;
	pthread_mutex_t m_mutex;
};

//-----------------------------------------------------------------------
Callisto_WaitEvent::Callisto_WaitEvent()
{
	m_implementation = new Callisto_WaitEvent_implementation;
	Callisto_WaitEvent_implementation *I = (Callisto_WaitEvent_implementation *)m_implementation;
	pthread_cond_init( &I->m_condition, 0 );
	pthread_mutex_init( &I->m_mutex, 0 );
}

//------------------------------------------------------------------------------
Callisto_WaitEvent::~Callisto_WaitEvent()
{
	Callisto_WaitEvent_implementation *I = (Callisto_WaitEvent_implementation *)m_implementation;

	pthread_cond_destroy( &I->m_condition );
	pthread_mutex_destroy( &I->m_mutex );
	delete I;
}

//------------------------------------------------------------------------------
void Callisto_WaitEvent::signal()
{
	Callisto_WaitEvent_implementation *I = (Callisto_WaitEvent_implementation *)m_implementation;

	pthread_mutex_lock( &I->m_mutex );
	pthread_cond_signal( &I->m_condition );
	pthread_mutex_unlock( &I->m_mutex );
}

//------------------------------------------------------------------------------
void Callisto_WaitEvent::wait( const unsigned int milliseconds )
{
	Callisto_WaitEvent_implementation *I = (Callisto_WaitEvent_implementation *)m_implementation;

	pthread_mutex_lock( &I->m_mutex );

	struct timespec abstime;

	clock_gettime( CLOCK_REALTIME, &abstime );
	abstime.tv_nsec += (milliseconds % 1000) * 1000000;
	abstime.tv_sec += milliseconds / 1000;

	if ( abstime.tv_nsec >= 1000000000 )
	{
		abstime.tv_nsec -= 1000000000;
		abstime.tv_sec += 1;
	}

	int err;
	do
	{
		err = pthread_cond_timedwait( &I->m_condition, &I->m_mutex, &abstime );

		if (
#ifdef ETIME
			  err == ETIME ||
#endif
			  err == ETIMEDOUT)
		{
			break;
		}

	} while ( err == EINTR || err == EAGAIN );

	pthread_mutex_unlock( &I->m_mutex );
}


//------------------------------------------------------------------------------
int64_t Callisto_getEpoch()
{
	return (int64_t)time(0);
}

//------------------------------------------------------------------------------
int64_t Callisto_getCurrentMilliseconds()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (int64_t)((tv.tv_usec/1000) + (tv.tv_sec * 1000));
}

}
