/*~
1
2
ARRAY 1
NULL
1
2
ARRAY 1
NULL
1
2
ARRAY 1
NULL
1
1
2
ARRAY 1
1
2
3
ARRAY 3
ARRAY 1
NULL
1
two
3
0:1
0:1
1:2
2:ARRAY 1
0:1
1:2
2:3
3:ARRAY 3
4:ARRAY 1
0:NULL
1:1
2:two
3:3
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



foreach( value, key : array_1 )
{
	log( key, ":", value );
}
foreach( value, key : array_2 )
{
	log( key, ":", value );
}
foreach( value, key : array_3 )
{
	log( key, ":", value );
}
foreach( value, key : array_4 )
{
	log( key, ":", value );
}
foreach( value, key : array_5 )
{
	log( key, ":", value );
}





//array[2] = 3;
//array[1] = array[2];
//log( array[2] );
//log( array[1] );

/*
array[2]++;
++array[2];
array[2] += 4;

table += [ "element":null ];
array += "something";
array += var;
*/
/*



foreach( value : array )
{
log( value )
}
*/


