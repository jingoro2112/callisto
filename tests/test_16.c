/*~
1
2
3
1
2
3
1
2
3
1
2
3
4
~*/


log( f1() );
f = new f1();
log( f.f1a() );
log( f.f1a.f1b() );

log( f2() );
f = new f2();
log( f.f2a() );
log( f.f2a.f2b() );

log( f3() );
f = new f3();
log( f.f3a() );
log( f.f3a.f3b() );

log( f4() );
f = new f4();
log( f.f4a() );
log( f.f4a.f4b() );
log( f.f4a.f4c() );

unit f1()
{
	while( 1 )
		while( 2 )
		{
			while( 3 )
			{
				return 1;
			}
		}

	unit f1a()
	{
		while( 1 )
		{
			while( 2 )
			{
				while( 3 )
				{
					return 2;
				}
			}
		}

		unit f1b()
		{
			while( 1 )
			{
				while( 2 )
				{
					while( 3 )
					{
						return 3;
					}
				}
			}
		}
	}
}

unit f2()
{
	for(;;)
		for(;;)
			for(;;)
				return 1;
	unit f2a()
	{
		for(;;)
			for(;;)
				for(;;)
					return 2;

		unit f2b()
		{
			for(;;)
				for(;;)
					for(;;)
						return 3;
		}
	}
}

unit f3()
{
	do {
		do 
			do {
				return 1;
			} while(1);
		 while(1);
	} while(1);


	unit f3a()
	{
		do {
			do {
				do 
					return 2;
				 while(1);
			} while(1);
		} while(1);

		unit f3b()
		{
			do 
				do
					do {
						return 3;
					} while(1);
				 while(1);
			 while(1);
		}
	}
}

unit f4()
{
	if ( 1 )
	{
		if ( 2 )
		{
			if ( 3 )
			{
				return 1;
			}
		}
	}

	unit f4a()
	{
		if ( 0 )
		{
		}
		else
		{
			if ( 1 )
			{
				return 2;
			}
		}

		unit f4b()
		{
			if ( 0 )
			{
			}
			else
			{
				if ( 0 )
				{
				}
				else if ( 1 )
				{
					return 3;
				}
			}
		}

		unit f4c()
		{
			if ( 0 )
			{
			}
			else
			{
				if ( 0 )
				{
				}
				else if ( 0 )
				{
					return 3;
				}
			}
			return 4;
		}
	}
}
