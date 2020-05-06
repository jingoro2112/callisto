/*~
10
5
track panes: 14
chair and desk
furniture was called
chair and desk
furniture was called
house chair and desk 2 34
15
15
25
25
16
16
26
26
1
2
3
1
2
3
1
2
3
1
2
3
4
yay
1
in pf
called
test
b
c
NULL
d
e
2
9
9
10
19
11
20

40

~*/

a = 5;
h = new test();

unit test()
{
	a = 10;
	log( a );
	log( ::a );
}

h = new House(14);

unit Window( panes )
{
	trackPanes = panes;
	log( "track panes: ", trackPanes );
}

unit Furniture()
{
	chair = 1;
	desk = 2;
	log( "chair and desk" );

	unit calledInFurniture()
	{
		log("furniture was called");
	}
}

unit House( panesOfGlass ) : Window, Furniture::calledInFurniture
{
	f = new Furniture::calledInFurniture();

	trackPanes += 20;
	chair++;

	log( "house chair and desk ", chair, " ", trackPanes );

	unit Home() : Furniture
	{
		log( "Home furniture" );
	}

	myLocal = 30;
}

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


log( f1() );
f = new f1();
log( f.f1a() );
log( f.f1a.f1b() );

log( f2() );
f = new f2();
log( f.f2a() );
log( f.f2a.f2b() );

log( f3() );
f = new f3();
log( f.f3a() );
log( f.f3a.f3b() );

log( f4() );
f = new f4();
log( f.f4a() );
log( f.f4a.f4b() );
log( f.f4a.f4c() );

unit f1()
{
	while( 1 )
		while( 2 )
		{
			while( 3 )
			{
				return 1;
			}
		}

	unit f1a()
	{
		while( 1 )
		{
			while( 2 )
			{
				while( 3 )
				{
					return 2;
				}
			}
		}

		unit f1b()
		{
			while( 1 )
			{
				while( 2 )
				{
					while( 3 )
					{
						return 3;
					}
				}
			}
		}
	}
}

unit f2()
{
	for(;;)
		for(;;)
			for(;;)
				return 1;
	unit f2a()
	{
		for(;;)
			for(;;)
				for(;;)
					return 2;

		unit f2b()
		{
			for(;;)
				for(;;)
					for(;;)
						return 3;
		}
	}
}

unit f3()
{
	do {
		do 
			do {
				return 1;
			} while(1);
		while(1);
	} while(1);


	unit f3a()
	{
		do {
			do {
				do 
					return 2;
				while(1);
			} while(1);
		} while(1);

		unit f3b()
		{
			do 
				do
					do {
						return 3;
					} while(1);
			while(1);
			while(1);
		}
	}
}

unit f4()
{
	if ( 1 )
	{
		if ( 2 )
		{
			if ( 3 )
			{
				return 1;
			}
		}
	}

	unit f4a()
	{
		if ( 0 )
		{
		}
		else
		{
			if ( 1 )
			{
				return 2;
			}
		}

		unit f4b()
		{
			if ( 0 )
			{
			}
			else
			{
				if ( 0 )
				{
				}
				else if ( 1 )
				{
					return 3;
				}
			}
		}

		unit f4c()
		{
			if ( 0 )
			{
			}
			else
			{
				if ( 0 )
				{
				}
				else if ( 0 )
				{
					return 3;
				}
			}
			return 4;
		}
	}
}


unit t()
{
	log("yay");
	return 1;
}

log( ::t() );

unit pf( pfarg )
{
	log( "in pf" );
	log( pfarg );
}

unit someOther( soarg )
{
	soarg( "called" );
}

someOther( pf );

f = new statef1();
log( "test" );
f.f1a("b");
f.f1a.f11a("c");
f.f1a.f11a("d", "e");

f.f1a(2);
f.f1a( 2 + 3 + 4 );
log( f.f1b + 4 );
f.f1c = 10;
log( f.f1c );

unit statef1()
{
	unit f1a( b )
	{
		log( b );

		unit f11a( c, d )
		{
			log( c );
			log( d );
		}
	}

	f1b = 5;
}

unit state( initialValue )
{
	if ( initialValue != null )
	{
		a = initialValue;
	}

	++a;
}

tr = new state(15);
tr();
tr();
tr();
log( tr.a );
tr.a = 10;
tr();
log( tr.a );


unit Parent1( init )
{
	a = init;
}

unit Parent2( init )
{
	b = init * 2;
}

unit SomeUnit( init ) : Parent1, Parent2
{
	log( a + "\n" );
	log( b + "\n" );
}

S = new SomeUnit( 20 );
