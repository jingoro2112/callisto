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
string
string
987
987
string
987
string
987
string
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
string
9876
string
9876
string
09876
string
string
9876
string
9876
string
09876
string
~*/

//٩(-̮̮̃-̃)۶٩(●̮̮̃•̃)۶٩(͡๏̯͡๏)۶٩(-̮̮̃•̃).𠑗𧅄𣔕𤧩
g = "10";

uni1 = "٩(-̮̮̃-̃)۶٩(●̮̮̃•̃)۶٩(͡๏̯͡๏)۶٩(-̮̮̃•̃).𠑗𧅄𣔕𤧩";
uni2 = "٩(-̮̮̃-̃)۶٩(●̮̮̃•̃)۶٩(͡๏̯͡๏)۶٩(-̮̮̃•̃).𠑗𧅄𣔕𤧩";
if ( uni1 != uni2 || uni1.length() != 87 )
{
	log("err");
}

a = 10;
log( a );
log( stlib::typeof(a) );
b = (string)a;
log( stlib::typeof(b) );
log( stlib::typeof(a) );
log( b );

b = (int)b;
log( b );
log( stlib::typeof(b) );

if ( b == 10 )
{
	log("yes");
}
else
{
	log("no");
}

w = (string)a;
log(w);
log(stlib::typeof(w));
l = "987";
log(stlib::typeof(l));
log(l);

log((int)l);
log(stlib::typeof(l));
log((float)l);
log(stlib::typeof(l));
log((string)l);
log(stlib::typeof(l));

i = 789;
log(stlib::typeof(i));
log((int)i);
log(stlib::typeof(i));
log((float)i);
log(stlib::typeof(i));
log((string)i);
log(stlib::typeof(i));

j = 789.123;
log(stlib::typeof(j));
log((int)j);
log(stlib::typeof(j));
log((float)j);
log(stlib::typeof(j));
log((string)j);
log(stlib::typeof(j));

k = "09876";
log(stlib::typeof(k));
log((int)k);
log(stlib::typeof(k));
log((float)k);
log(stlib::typeof(k));
log((string)k);
log(stlib::typeof(k));

l = "09876";
log(stlib::typeof(l));
log((int)l);
log(stlib::typeof(l));
log((float)l);
log(stlib::typeof(l));
log((string)l);
log(stlib::typeof(l));
