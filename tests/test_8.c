/*~
10
integer
string
integer
10
10
integer
yes
10
wstring
wstring
987
987
wstring
987
wstring
987
wstring
987
wstring
integer
789
integer
789
integer
789
integer
789
integer
float
789
float
789.123
float
789.123
float
789.123
float
string
9876
string
9876
string
09876
string
09876
string
wstring
9876
wstring
9876
wstring
09876
wstring
09876
wstring
~*/

a = 10;
log( a );
log( typeof(a) );
b = (string)a;
log( typeof(b) );
log( typeof(a) );
log( b );

b = (int)b;
log( b );
log( typeof(b) );

if ( b == 10 )
{
	log("yes");
}
else
{
	log("no");
}

w = (wstring)a;
log(w);
log(typeof(w));
l = L"987";
log(typeof(l));
log(l);

log((int)l);
log(typeof(l));
log((float)l);
log(typeof(l));
log((string)l);
log(typeof(l));
log((wstring)l);
log(typeof(l));

i = 789;
log(typeof(i));
log((int)i);
log(typeof(i));
log((float)i);
log(typeof(i));
log((string)i);
log(typeof(i));
log((wstring)i);
log(typeof(i));

j = 789.123;
log(typeof(j));
log((int)j);
log(typeof(j));
log((float)j);
log(typeof(j));
log((string)j);
log(typeof(j));
log((wstring)j);
log(typeof(j));

k = "09876";
log(typeof(k));
log((int)k);
log(typeof(k));
log((float)k);
log(typeof(k));
log((string)k);
log(typeof(k));
log((wstring)k);
log(typeof(k));

l = L"09876";
log(typeof(l));
log((int)l);
log(typeof(l));
log((float)l);
log(typeof(l));
log((string)l);
log(typeof(l));
log((wstring)l);
log(typeof(l));
