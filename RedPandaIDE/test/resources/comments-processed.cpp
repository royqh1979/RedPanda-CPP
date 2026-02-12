#define TEST 1+ 2 4




int foo = 1;

int bar = 0xE +foo; 

int baz = 0xE + foo; 
int pub = bar+++baz; 
int ham = bar++-++baz; 

int qux = bar+++ ++baz; 