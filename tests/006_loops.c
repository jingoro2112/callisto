/*~
out 1 11
out 2 12
out 2 13
loop10
loop13
loop16
loop19
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
131
123
235
347
459
5611
6713
7815
8917
91019
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



for( tristan = 1; tristan <= 7; tristan++ )
{
	log( "test.log", "tristan is: ", tristan );
	if ( tristan <= 6 )
	{
		log( "test.log", "line feed follows!");
	}
}


i = 0;
for(;;)
{
	if ( ++i == 10 )
	{
		break;
	}
}

for( i=100;;)
{
	if ( ++i == 110 )
	{
		break;
	}
}

for(;i<120;)
{
	++i;
}

for(;;++i)
{
	if ( i > 130 )
	{
		break;
	}
}

log(i);

for( i=1, j=2, k=3; i<10; ++i, ++j, k = j + i )
{
	log(i,j,k);
}


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
