/*~
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
16.3818
90.1
12
34
56
78
910
and
567
fourNULL
TWONULLthreeNULL
ONE
NULL
some stringstring 1 and thisand some more?thisworks?
string 1 and thisand some more?
n2
n4
~*/

a = null;

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

d = 90.1;
log( d /= 5.5 );
log( d *= 5.5 );

log( "1" + 2 );
log( "3" + 4.0 );
log( "5" + "6" );
log( 7 + "8" );
log( 9.0 + "10" );

log( "ONE\n", log("TWO", log("and"), "three", log("four", log(5,6,7))));

s1 = "string 1 and this""and some more?";
s2 = "some string" + s1 + "thisworks?";
log( s2 );
s2 += " and another";
log( s1 );

if ( null == 0 )
{
	log( "n1" );
}

if ( null == null )
{
	log( "n2" );
}

if ( 0 == null )
{
	log( "n3" );
}

if ( 0 == 0 )
{
	log( "n4" );
}

if ( null > null )
{
	log( "n5" );
}