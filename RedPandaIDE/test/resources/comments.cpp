#define TEST 1+/*
test*/2/* test
sadfasdf
*/4

int foo = 1;
// int bar = 0xE+foo; // error: invalid preprocessing number 0xE+foo
int bar = 0xE/*Comment expands
 to a space*/+foo; // OK: 0xE + foo
int baz = 0xE + foo; // OK: 0xE + foo
int pub = bar+++baz; // OK: bar++ + baz
int ham = bar++-++baz; // OK: bar++ - ++baz
// int qux = bar+++++baz; // error: bar++ ++ +baz, not bar++ + ++baz
int qux = bar+++/*Saving comment*/++baz; // OK: bar++ + ++baz