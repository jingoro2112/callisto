/*~
4
2
vtest
70
2
vtest
5
5
copy
5
5
6
2
~*/

var = "vtest";
table1 = [ 1:4, 2:2, "this":var, 20:40 + 50 - 10 * 2];
log( table1[1] );
log( table1[2] );
log( table1["this"] );
log( table1[20] );

log( table1[table1[table1[table1[table1[2]]]]] );

table2 = table1;

log( table2["this"] );

table2[1] = 5;
log( table1[1] );
log( table2[1] );

table3 = copy table2;
log("copy");
table3[1] = 6;
log( table1[1] );
log( table2[1] );
log( table3[1] );

log( table1[table2[table3[table2[table1[2]]]]] );
