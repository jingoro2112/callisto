/*~
yes
no
no
yes
yes2
null okay
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



l = null;
if ( l != null )
{
	log( "l wasn't null" );
}
if ( null != l )
{
	log( "l2 wasn't null" );
}
if ( null == null )
{
	log( "null okay" );
}
if ( null != null )
{
	log( "null NOT okay" );
}
