/*~
0
2
6
-1
6
6
~*/


log( TestEnums1::name1 );
log( TestEnums2::name2 );
log( U::e2::value2 );

U();

unit U()
{
	log( TestEnums7::name10 );

	enum e2
	{
		value2 = 6
	}

	log( U::e2::value2 );
	log( e2::value2 );
}

enum TestEnums1
{
	name1
}

enum TestEnums2
{
	name2 = 2
}

enum TestEnums3
{
	name3 = 3,
}

enum TestEnums4
{
	name4 = 4,
	name5
}

enum TestEnums5
{
	name5 = 5,
	name6 = 6
}

enum TestEnums6
{
	name6 = 6,
	name7 = 6,
}

enum TestEnums7
{
	name6 = 6,
	name7 = 6,
	name8,
	name9,
	name10 = -1,
	name11,
}

