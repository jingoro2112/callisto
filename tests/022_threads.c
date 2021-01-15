/*~
100
1
1
msg1 1
200
1__23
1
6
11
~*/

e = sys::epoch();
wait( c_wait, 100 ); // make sure this task waits for the c routine to finish
e = sys::epoch() - e;
log( e );

e = sys::epoch();
wait( sleepTask );
e = sys::epoch() - e;
log( e );

unit sleepTask()
{
	sleep( 1000 );
}

glob = 1;

someTask("msg1", 0);

e = sys::epoch();

wait( ::c_wait, 200 );
wait( someTask::doSomeSleeping, "1_", "_2", 3 );

e = sys::epoch() - e;
log( e );

unit someTask( message, delay )
{
	log( message, " ", ::glob );
	sleep( delay );
	
	unit doSomeSleeping( a, b, c )
	{
		log( a, b, c );
		sys::sleep( 1000 );
	}
}

unit incGlob( a, b )
{
	::glob += a;
}

t = thread( incGlob );
yield();

t = thread( incGlob, 1 );
yield();

t = thread( incGlob, 0, 3 );
yield();

t = thread( incGlob, 4, 0 );
yield();

log( glob );

thread( incGlob );
thread( incGlob, 1 );
thread( incGlob, 0, 3 );
thread( incGlob, 4, 0 );
yield();

log( glob );
sleep(100);