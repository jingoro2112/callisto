/*~
track panes: 14
chair and desk
furniture was called
chair and desk
furniture was called
house chair and desk2 34
~*/

h = new House(14);

unit Window( panes )
{
	trackPanes = panes;
	log( "track panes: ", trackPanes );
}

unit Furniture()
{
	chair = 1;
	desk = 2;
	log( "chair and desk" );

	unit calledInFurniture()
	{
		log("furniture was called");
	}
}

unit House( panesOfGlass ) : Window, Furniture::calledInFurniture
{
	f = new Furniture::calledInFurniture();

	trackPanes += 20;
	chair++;

	log( "house chair and desk", chair, " ", trackPanes );

	unit Home() : Furniture
	{
		log( "Home furniture" );
	}

	myLocal = 30;
}
