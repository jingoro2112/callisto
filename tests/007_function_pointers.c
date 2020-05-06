/*~
hello
hello
hi
hi
~*/

// function pointers

unit test2( initialValue )
{
	log( "hello");

	unit test22()
	{
		log( "hi" );
	}
}

unit test3( t2 )
{
	t2();
}

a = test2;
test3( a );

a = ::test2;
test3( a );

a = test2.test22;
test3( a );

a = test2::test22;
test3( a );

