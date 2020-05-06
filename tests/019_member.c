/*~
5
10
10
3
4
4
32
~*/

a = 5;
member unit test1()
{
	a = 10;
}

unit test2()
{
	a = 3;
}

log( a );
test1();
log( a );
test2();
log( a );

unit test3()
{
	b = 3;
	member test4()
	{
		b = 4;

		unit test5()
		{
			b = 5;
		}
	}
}

t = new test3();
log( t.b );
t.test4();
log( t.b );
t.test4.test5();
log( t.b );


unit vector( x0, y0, z0 )
{
	x = x0;
	y = y0;
	z = z0;

	// without the "member" designator the x/y/z would be null 
	member unit dotProduct( other )
	{
		return x*other.x + y*other.y + z*other.z;
	}
}

v1 = new vector( 1, 2, 3 );
v2 = new vector( 4, 5, 6 );
log( v1.dotProduct(v2) );
