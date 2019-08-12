/*~
yes
no
no
yes
yes2
0
1
2
3
4
5
6
7
8
9
10
10
11
12
10
~*/


a = 10;

if ( a == 10 )
	log( "yes" );
else
	log( "no" );


if ( a != 10 )
	log( "yes" );
else
{
	log( "no" );
}

if ( a != 10 )
{
	log( "yes" );
}
else
	log( "no" );

if ( a == 10 )
	log( "yes" );

log("yes2");

for( i=0; i<10; ++i )
	log(i);
log(i);

do
{
	log( i );
	++i;
} while ( i < 13 );

do
	--i;
while( i >10 );

log(i);
