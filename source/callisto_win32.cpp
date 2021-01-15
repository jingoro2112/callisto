#include <windows.h>
#include <process.h>

#include "vm.h"

namespace Callisto
{

//------------------------------------------------------------------------------
void Callisto_sleep( const int milliseconds )
{
	Sleep( milliseconds );
}

//------------------------------------------------------------------------------
int64_t Callisto_getEpoch()
{
	return (int64_t)time(0);
}

//------------------------------------------------------------------------------
static void callInThread_EX( void* arg )
{
	Callisto_ExecutionContext *exec = (Callisto_ExecutionContext *)arg;

	exec->function( exec );
	exec->state = Callisto_ThreadState::FromC | Callisto_ThreadState::Runnable;

	exec->callisto->OSWaitHandle.signal();
}

//------------------------------------------------------------------------------
int Callisto_startThread( Callisto_ExecutionContext* exec )
{
	_beginthread( callInThread_EX, 0, exec );

	return CE_NoError;
}

//-----------------------------------------------------------------------
Callisto_WaitEvent::Callisto_WaitEvent()
{
	m_implementation = CreateEvent( NULL, true, false, NULL );
}

//-----------------------------------------------------------------------
Callisto_WaitEvent::~Callisto_WaitEvent()
{
	CloseHandle( (HANDLE)m_implementation );
}

//-----------------------------------------------------------------------
void Callisto_WaitEvent::signal()
{
	SetEvent( (HANDLE)m_implementation );
}

//-----------------------------------------------------------------------
void Callisto_WaitEvent::wait( const unsigned int milliseconds )
{
	WaitForSingleObject( (HANDLE)m_implementation, milliseconds );
}

//------------------------------------------------------------------------------
int64_t Callisto_getCurrentMilliseconds()
{
	FILETIME ft;
	GetSystemTimeAsFileTime( &ft );
	ULARGE_INTEGER ftui;
	ftui.LowPart = ft.dwLowDateTime;
	ftui.HighPart = ft.dwHighDateTime;
	return ftui.QuadPart / 10000;
}

}