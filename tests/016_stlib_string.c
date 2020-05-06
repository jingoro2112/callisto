/*~
ltrim  
this
is
a
string
this is a string
this
is
a
string
this is a string
1
1
1
1
1
1
1
1
12345
1234
234
abcde
abcde
abcdefghij
abcde
145678
12_1212_
8
NULL
10
8
ABCDEFGHIJIJ
abcdefghijij
1
0
1
1
1
1
1
1
1
1
0
23
0123456789
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
a
s
t
r
i
n
g
23
  23
12345678
8
8
NULL
10
8
1
0
1
1
1
1
1
1
1
1
0
23
0123456789
~*/

s1 = "  ltrim  ";

s1.ltrim();
log( s1 );

s = "this is a string";
f = s.split();
foreach( term : f )
{
	log( term );
}
log( s );
f = string::split( s );
foreach( term : f )
{
	log( term );
}
log( s );


s1 = "  ltrim  ";
s2 = "  rtrim  ";
s1.ltrim();
s2.rtrim();
log( s1 == "ltrim  " );
log( s2 == "  rtrim" );
s1.ltrim();
s2.rtrim();
log( s1 == "ltrim  " );
log( s2 == "  rtrim" );

s1.trim();
log( s1 == "ltrim" );
s2.trim();
log( s2 == "rtrim" );

s1.trim();
log( s1 == "ltrim" );
s2.trim();
log( s2 == "rtrim" );

s1 = "123456789";
s1.truncate( 5 );
log( s1 );
s1.shave( 1 );
log( s1 );
s1.shift( 1 );
log( s1 );

s1 = "abcdefghij";
log( a = string::truncate( s1, 5 ) );
log( a );
log( s1 );
string::shave( a, 1 );
log( a );

s1 = "12345678";
s1.shift( 3, 1 );
log( s1 );

s1 = "ab_abab_";
s1.replace( "ab", "12" );
log( s1 );

log( s1.length() );

s1 = "abCDefghIJij";
log( s1.find("not") );
log( s1.find("ij") );
log( s1.find("ij", false) );

log( s1.upper().lower().upper().lower().upper() );
log( s1.lower() );

s1 = "0123456789";
log( s1.substring() == "" );
log( s1.substring(0,0).length() );
log( s1.substring(0,1).length() );
log( s1.substring(0,1) == "0" );
log( s1.substring(1,2) == "1" );
log( s1.substring(2,0) == null );
log( s1.substring(2,4) == "23" );
log( s1.substring(8,9) == "8" );
log( s1.substring(9,10) == "9" );
log( s1.substring(0,10) == "0123456789" );
log( s1.substring(0,1) );
log( s1.substring(2,4) );
log( s1.substring(0,10) );

s1 = s1.toarray();
foreach( character : s1 )
{
	log( character );
}

s1 = "astring";
s2 = string::toarray( s1 );
foreach( character : s2 )
{
	log( character );
}

s1 = "  23";
log( string::ltrim(s1) );
log( s1 );

s1 = "12345678";
string::shift( s1, 3, 1 );
log( s1 );

log( len = string::length(s1) );
log( len );

s1 = "abCDefghIJij";
log( string::find(s1, "not") );
log( string::find(s1, "ij") );
log( string::find(s1, "ij", false) );

s1 = "0123456789";
log( string::substring(s1) == "" );
log( string::substring(s1,0,0).length() );
log( string::substring(s1,0,1).length() );
log( string::substring(s1,0,1) == "0" );
log( string::substring(s1,1,2) == "1" );
log( string::substring(s1,2,0) == null );
log( string::substring(s1,2,4) == "23" );
log( string::substring(s1,8,9) == "8" );
log( string::substring(s1,9,10) == "9" );
log( string::substring(s1,0,10) == "0123456789" );
log( string::substring(s1,0,1) );
log( string::substring(s1,2,4) );
log( string::substring(s1,0,10) );

