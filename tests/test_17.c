/*~
0
1
1
2
3
5
8
13
21
34
55
89
144
233
377
610
987
1597
2584
4181
~*/

unit fibonacci( n )
{
	if (n <= 1) return n;
	return fibonacci(n - 2) + fibonacci(n - 1);
}

for ( i = 0; i < 20; i++)
{
	log( fibonacci(i) );
}
