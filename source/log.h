#ifndef LOG_HPP
#define LOG_HPP
/*------------------------------------------------------------------------------*/

// log is NOT dependant on any of the other libraries in common so it
// can be used without any fuss

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#pragma warning (disable : 4996) // remove Windows nagging about using _strincmp() etc
#else
#include <unistd.h>
#include <sys/time.h>
#endif

//------------------------------------------------------------------------------
// ultra-simple lightweight logging facility to get directly at
// low-level spammy log messages.
// acceptable log levels are 0-31 where default level is '0' which is
// also default enabled, the remaining 30 are default off.

/*

example callbacks:
 
Windows:
#include <process.h>

const unsigned int MillisecondTickCallback()
{
	return (unsigned int)GetTickCount();
}

const unsigned int PIDCallback()
{
	return _getpid();
}

const unsigned int TIDCallback()
{
	return GetCurrentThreadId();
}

*nix:
#include <sys/syscall.h> 
#include <sys/types.h>
#include <unistd.h>

const unsigned int MillisecondTickCallback()
{
	struct timeval tv;
	gettimeofday( &tv, 0 );
	return (unsigned int)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

const unsigned int PIDCallback()
{
	return getpid();
}

const unsigned int TIDCallback()
{
	return syscall(SYS_gettid);
}

void TMCallback( tm& timeStruct )
{
	time_t tod = time(0);
	memcpy( &timeStruct, localtime(&tod), sizeof(struct tm) );
}

void loggingCallback( const char* msg )
{
	printf( "%s\n", msg ); // %s important so '%' characters are not interpretted by the printf
}


CLog Log
Log.setMillisecondsCallback( MillisecondTickCallback );
Log.setPIDCallback( PIDCallback );
Log.setTIDCallback( TIDCallback );
Log.setTMCallback( TMCallback );
Log.setCallback( loggingCallback );

*/

//------------------------------------------------------------------------------
class CLog
{
public:

	enum
	{
		PID =1<<0,
		TID =1<<1,
		VERBOSE =1<<2,
		UNDECORATED =1<<3
	};
	
	void setLogFlags( const unsigned int flags =CLog::VERBOSE ) { m_logFlags = flags; }
	void setFile( const char *path, bool clobber =false )
	{
		strcpy( m_filename, path );
		if ( clobber )
		{
#ifdef _WIN32
			_unlink( m_filename );
#else
			unlink( m_filename );
#endif
		}
	}

	// where the logs will go, default is printf
	void setCallback( void (*callback)(const char* msg) ) { m_callback = callback; }

	// system-specific calls to get the logging information, TM is the
	// only only one that has a default (localtime())
	void setMillisecondsCallback( const unsigned int (*callback)(void) ) { m_millisecondsCallback = callback; }
	void setPIDCallback( const unsigned int (*callback)(void) ) { m_pidCallback = callback; }
	void setTIDCallback( const unsigned int (*callback)(void) ) { m_tidCallback = callback; }
	void setTMCallback( void (*callback)( tm& timeStruct ) ) { m_tmCallback = callback; }
	
	void enableLevel( const unsigned int level, const bool enable ) { enable ? (m_enabledVector |= 1 << ((level > 31) ? 31 : level)) : (m_enabledVector &= ~(1 << ((level > 31) ? 31 : level))); }
	void enableUpToInclusive( const unsigned int level, const bool enable ) { for ( unsigned int i=0; i<=level; i++ ) { enableLevel(i, enable); } }

	void operator()( const int level, const char* format, ... )
	{
		if ( enabled(level) )
		{
			va_list arg;
			va_start( arg, format );
			outEx( format, arg );
			va_end( arg );
		}
	}
	
	void operator()( const char* format, ... )
	{
		if ( m_enabledVector & 0x1 )
		{
			va_list arg;
			va_start( arg, format );
			outEx( format, arg );
			va_end( arg );
		}
	}

	void out( const int level, const char* format, ... )
	{
		if ( enabled(level) )
		{
			va_list arg;
			va_start( arg, format );
			outEx( format, arg );
			va_end( arg );
		}
	}

	void out( const char* format, ... )
	{
		if ( m_enabledVector & 0x1 )
		{
			va_list arg;
			va_start( arg, format );
			outEx( format, arg );
			va_end( arg );
		}
	}

	CLog()
	{
		m_filename[0] = 0;
		m_callback = simpleOut;
		m_logFlags = CLog::VERBOSE;
		m_enabledVector = 0x1;
		m_millisecondsCallback = 0;
		m_pidCallback = 0;
		m_tidCallback = 0;
		m_tmCallback = 0;  
	}
	
private:

	static void simpleOut( const char* text ) { printf( "%s\n", text ); }
	
	bool enabled( const unsigned int level ) const { return (m_enabledVector & (1<< ((level > 31) ? 31 : level) )) ? true : false; }

	void outEx( const char* format, va_list arg )
	{
		struct tm tmbuf;
		if ( m_tmCallback )
		{
			m_tmCallback( tmbuf );
		}
		else
		{
			time_t tod = time(0);
			memcpy( &tmbuf, localtime(&tod), sizeof(struct tm) );
		}
		char temp[64001];

		unsigned int tick = m_millisecondsCallback ? m_millisecondsCallback() : 0;
		
		unsigned int pidAdd = 0;
		if ( (m_logFlags & CLog::PID) && m_pidCallback )
		{
			sprintf( temp, "P%03d ", m_pidCallback() % 1000 );
			if ( (m_logFlags & CLog::TID) && m_tidCallback )
			{
				sprintf( temp + 5, "T%03d ", m_tidCallback() % 1000 );
				pidAdd = 10;
			}
			else
			{
				pidAdd = 5;
			}
		}
		else if ( (m_logFlags & CLog::TID) && m_tidCallback )
		{
			pidAdd = 5;
			sprintf( temp, "T%03d ", m_tidCallback() % 1000 );
		}
			 
		int len;
		if ( m_logFlags & CLog::VERBOSE )
		{
			sprintf( temp + pidAdd, "%02d %02d:%02d:%02d.%03d> ",
					 tmbuf.tm_mday,
					 tmbuf.tm_hour,
					 tmbuf.tm_min,
					 tmbuf.tm_sec,
					 tick % 1000 );
			
			pidAdd += 17;
		}
		else if ( !(m_logFlags & CLog::UNDECORATED) )
		{
			sprintf( temp + pidAdd, "%02d.%03d> ",
					 tmbuf.tm_sec,
					 tick % 1000 );
			
			pidAdd += 8;
		}

		len = vsnprintf( temp + pidAdd, 64000, format, arg ) + pidAdd;

		if ( temp[len - 1] == '\n' )
		{
			len--;
			temp[len] = 0;
		}

		if ( m_callback )
		{
			m_callback( temp );
		}

		if ( m_filename[0] )
		{
			FILE *fil = fopen( m_filename, "a" );
			if ( fil )
			{
				temp[len++] = '\n';
				temp[len] = 0;

				fwrite( temp, len, 1, fil );
				fclose( fil );
			}
		}
	}

	void (*m_callback)(const char* msg);
	const unsigned int (*m_millisecondsCallback)(void);
	const unsigned int (*m_pidCallback)(void);
	const unsigned int (*m_tidCallback)(void);
	void (*m_tmCallback)( tm& timeStruct );
	char m_filename[256];
	unsigned int m_logFlags;
	unsigned int m_enabledVector;
};

#endif
