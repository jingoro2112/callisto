/*~
-10
10
26
27
28
33
120
19
31
32
33
some stringstring 1 and thisand some more?thisworks?
string 1 and thisand some more?
and
567
fourNULL
TWONULLthreeNULL
ONE
NULL
out 1 11
out 2 12
out 2 13
loop10
loop13
loop16
loop19
16.3818
90.1
test.logtristan is: 1
test.logline feed follows!
test.logtristan is: 2
test.logline feed follows!
test.logtristan is: 3
test.logline feed follows!
test.logtristan is: 4
test.logline feed follows!
test.logtristan is: 5
test.logline feed follows!
test.logtristan is: 6
test.logline feed follows!
test.logtristan is: 7
12
3
0
1
2
0
5
10
11
43981
11259375
20017429977206
1311862290986177367
1
1.2
1.89232
2
~*/

l = null;
if ( l != null )
{
	log( "l wasn't null" );
}

n = -10;
log(n);
n = -(-(-(-(-(-10)))));
log(n);


a = 25;
log( 1 + ++a - 1 );
log( ++a + 1 - 1 );
log( 1 - 1 + ++a );
log( 1 + a++ + 10 - 2 * 3 );
log( (40 + 50) * 4 / (((2 + 1))) );
log( 3 * 5 - 10 + 10 + 10 - 2 * 3 );
log( 1 + 1 + a++ );
log( a++ + 1 + 1 );
log( 1 + a++ + 1 );


s1 = "string 1 and this""and some more?";
s2 = "some string" + s1 + "thisworks?";
log( s2 );
s2 += " and another";
log( s1 );

log( "ONE\n", log("TWO", log("and"), "three", log("four", log(5,6,7))));


a = 10;
while( ++a < 14)
{
	if ( a < 12 )
	{
		log( "out 1 ", a );
		continue;
	}

	log( "out 2 ", a );
}

a = 10;
while( a < 20)
{
	log( "loop", a );
	++a;
	a += 1;
	a = a + 1;
}


a = 25;
d = 90.1;
log( d /= 5.5 );
log( d *= 5.5 );


for( tristan = 1; tristan <= 7; tristan++ )
{
	log( "test.log", "tristan is: ", tristan );
	if ( tristan <= 6 )
	{
		log( "test.log", "line feed follows!");
	}
}

log( 12 );
log( 3 );
log( 0 );
log( 0x1 );
log( 0x2 );
log( 0x );
log( 0b0101 );
log( 0xa );
log( 0xb );
log( 0xabcd );
log( 0xabcdef );
log( 0x1234abcd9876 );
log( 0x1234abcd98761357 );
log( 1.0 );
log( 1.2 );
log( 1.89232 );
log( 2. );

