/*~
loop
loop
loop
count: 3
count2: 3
1: 0
2: 0
2: 1
2: 2
count2: 3
1: 1
2: 0
2: 1
2: 2
count2: 3
1: 2
2: 0
2: 1
2: 2
count: 4
10 11
10 0
20 1
10 0
21 1
30 2
3
NULL
NULL
5
10
21
30
40
3:ARRAY[3]
sub: 0 1
sub: 1 2
sub: 2 3
2:two
sub: 0 116
sub: 1 119
sub: 2 111
1:1
3:ARRAY[3]
1:1
~*/

unit r1()
{
	.reset();
}

unit r2()
{
	r1();
}

unit r3()
{
	r2();
}

a1 = [0, 1, 2];
foreach( a : a1 )
{
	r3();
	log("loop");
}

a1 = [0, 1, 2];
log( "count: ", a1.count() );
foreach( v1 : a1 )
{
	log( "count2: ", .count() );
	log( "1: ", v1 );
	foreach( v2 : a1 )
	{
		log( "2: ", v2 );
	}
}

a1[3] = 3;
log( "count: ", a1.count() );

a1[1] = 11;
a1[0] = 10;
log( a1[0], " ", a1[1] );


a1 = [10,20,30];
foreach( k1, v1 : a1 )
{
	log( v1, " ", k1 );
	if ( v1 == 20 )
	{
		a1[k1] = 21;
		.reset();
	}
}

log( array::count(a1) );
log( array::insert(a1, 0, 5) );
log( array::add(a1, 40) );

foreach( v2 : a1 )
{
	log( v2 );
}

a0 = [1,2,3];
tb1 = [1:1, 2:"two", 3:a0];
foreach( k,v : tb1 )
{
	log( k, ":", v );
	foreach( k1,v1 : v )
	{
		log( "sub: ", k1, " ", v1 );
	}

	if ( k == 2 )
	{
		.remove( 2 );
	}
}

foreach( k,v : tb1 )
{
	log( k, ":", v );
}

