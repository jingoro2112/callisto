/*~
zero
zero
zero!
one
one
one
one
two
two
two
three
three
three
four
four
four
default
default
default
8
9
9
9
22
0
1
2
3
4
6
8
~*/

for( i=0; i<6; i++ )
{
	switch( i )
	{
		case 0: log("zero"); break;
		case 1: log("one"); break;
		case 2: log("two"); break;
		case 3: log("three"); break;
		case 4: log("four"); break;
		default: log("default"); break;
	}

	switch( i )
	{
		case 0: { log("zero"); break; }
		case 1: log("one"); break;
		case 2: { log("two"); break; }
		default: log("default"); break;
		case 3: log("three"); break;
		case 4: { log("four"); break; }
	}

	switch( i )
	{
		case 0:
		{
			switch( i )
			{
				case 0:
				{
					switch( i )
					{
						case 0:
							switch( i )
							{
								case 0:
								{
									log( "zero!" );
									break;
								}
							}
					}
				}
			}
		}
		
		case 1: log("one"); break;
		case 2: { log("two"); break; }
		default: log("default"); break;
		case 3: log("three"); break;
		case 4: { log("four"); break; }
	}

}

j = 0;
for( i=-2; i<10; ++i )
{
	switch( i )
	{
		case 1:
		case 2:
			j++;
		case 3:
			log(j);
			break;
		
		case 4:
		{
			log(j);
			break;
		}
	
		case 11:
		{
		}
		default:
		{
			switch( i )
			{
				case 9:
				{
					switch( i + 1 )
					{
						case 10:
							j++;
						default:
							break;
					}
				}
			}
		}

		case 5:
			j++;
		case 6:
		{
			j++;
		}
		case 7:
			j++;
			break;

		case EVals::val1:
		{
			j++;
			break;
		}

		case EVals::val2:
			j += 10;
			break;
	}

}

enum EVals
{
	val1 = -2,
	val2 = -3
}

log(j);

for( k = 5000000000 ; k < 5000000009; ++k )
{
	switch( k )
	{
		case 5000000000: log( "0" ); break;
		case 5000000001: log( "1" ); break;
		case 5000000002: log( "2" ); break;
		case 5000000003: log( "3" ); break;
		case 5000000004: log( "4" ); break;
		case 5000000006: log( "6" ); break;
		case 5000000008: log( "8" ); break;
		case 5000000009: log( "9" ); break;
	}
}
