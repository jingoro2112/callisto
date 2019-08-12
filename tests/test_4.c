/*~
I have been run with: bird
I have been run with: robin
I have been run with: 1
I have been run with: bird
bird
30
I have been run with: NULL
79
10
25
71
79
~*/

house( "bird" );
house( "robin" );
house( true );

h = new house( "bird" );
log( h.stories );

h.windows = 30;
log( h.windows );

w = new house::window();
log( w.f.flowerType );
log( w.f.grow() );
log( w.a + w.b * w.c + w.f.flowerType );

house::window();

unit house( arg )
{
	stories = arg;
	log( "I have been run with: ", stories );

	unit window()
	{
		a = 1;
		b = 2;
		c = 30;

		f = new FlowerPot();
		
		win_var = 79;
		log( win_var );

		// how is this named

		unit glass()
		{
			log( "and how am _I_ named" );
		}
	}
}

unit FlowerPot()
{
	flowerType = 10;
	unit grow()
	{
		return 25;
	}
			
}
