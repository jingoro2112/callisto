/*~
15
15
25
25
16
16
26
26
~*/

test1(15);
t = new test1(25);

unit test1( x )
{
	log( x );
	test2( x );
	
	unit test2( x )
	{
		log( x );
	}
}

test3(16);
t = new test3(26);

unit test3( copy x )
{
	log( x );
	test4( x );

	unit test4( copy x )
	{
		log( x );
	}
}
