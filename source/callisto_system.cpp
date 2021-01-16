#include "callisto.h"
#include "vm.h"

#include <stdio.h>

int64_t millisecondZero = 0;

//------------------------------------------------------------------------------
static Callisto_Handle c_epoch( Callisto_ExecutionContext* E )
{
	return Callisto_setValue( E, Callisto_getEpoch() );
}

//------------------------------------------------------------------------------
static Callisto_Handle c_milliseconds( Callisto_ExecutionContext* E )
{
	return Callisto_setValue( E, Callisto_getCurrentMilliseconds() - millisecondZero );
}

//------------------------------------------------------------------------------
static Callisto_Handle c_getLine( Callisto_ExecutionContext* E )
{
	C_str line;
	Callisto_getLine( line );
	return Callisto_setValue( E, line );
}

//------------------------------------------------------------------------------
static const Callisto_FunctionEntry Callisto_system[]=
{
	{ "sys::milliseconds", c_milliseconds, 0 },
	{ "sys::epoch", c_epoch, 0 },
	{ "sys::getline", c_getLine, 0 },
};

//------------------------------------------------------------------------------
void Callisto_importSystem( Callisto_Context *C )
{
	millisecondZero = Callisto_getCurrentMilliseconds();
	Callisto_importFunctionList( C, Callisto_system, sizeof(Callisto_system) / sizeof(Callisto_FunctionEntry) );
}
