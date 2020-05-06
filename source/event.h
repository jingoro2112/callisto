#ifndef EVENT_HPP
#define EVENT_HPP
/*------------------------------------------------------------------------------*/

#include "arch.h"

namespace Locks
{
	
//-----------------------------------------------------------------------
class Event
// this ensures that if an event triggers before the wait happens, that
// the wait will not block. if the reset is set to 'false' then the
// triggered state of the lock will NOT be consumed, for the case where
// many threads might potentiall wait on a condition is set true only
// once. In the case of 'broadcast' the state is never consumed, and
// reset MUST be explicitly called.
{
public:
	Event();
	~Event();

	void reset(); // 'wait' will now block 
	void signal(); // wake up one and ONLY one waiter (unpredictable which one)
	void broadcast(); // wake up all waiters
	void wait( bool reset =true ); // wait for signal or broadcast to occur, if reset is false, all other calls to wait will have no effect
	bool timedWait( unsigned int milliseconds, bool reset =true ); // allow a timeout

private:
	
#ifdef _WIN32
	HANDLE m_condition;
#else
	pthread_cond_t m_condition;
	pthread_mutex_t m_mutex;
	bool m_flag;
#endif

	bool m_broadcast;
};

}

#endif
