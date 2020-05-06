/*~
1
2
ARRAY[1]
NULL
1
2
ARRAY[1]
NULL
1
2
ARRAY[1]
NULL
1
1
2
ARRAY[1]
1
2
3
ARRAY[3]
ARRAY[1]
NULL
1
two
3
0:1
0:1
1:2
2:ARRAY[1]
0:1
1:2
2:3
3:ARRAY[3]
4:ARRAY[1]
0:NULL
1:1
2:two
3:3
2:two
1:one
0:zero
two
one
zero
~*/

array_1 = [];

array_2 = [1];
array_3 = [1,2,array_2];
array_4 = [1,2,3,array_3,array_2];
array_5 = [null,1,"two",3];


foreach( value : array_3 )
{
	foreach( value : array_3 )
	{
		log( value );
	}
	log( value );
}

foreach( value : array_1 )
{
	log( value );
}
foreach( value : array_2 )
{
	log( value );
}
foreach( value : array_3 )
{
	log( value );
}
foreach( value : array_4 )
{
	log( value );
}
foreach( value : array_5 )
{
	log( value );
}


foreach( key, value : array_1 )
{
	log( key, ":", value );
}

foreach( key, value : array_2 )
{
	log( key, ":", value );
}
foreach( key, value : array_3 )
{
	log( key, ":", value );
}
foreach( key, value : array_4 )
{
	log( key, ":", value );
}
foreach( key, value : array_5 )
{
	log( key, ":", value );
}

table_1 = [0:"zero",1:"one",2:"two"];
foreach( key, value : table_1 )
{
	log( key, ":", value );
}

table_1 = [0:"zero",1:"one",2:"two"];
foreach( value : table_1 )
{
	log( value );
}
