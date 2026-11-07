#include <DIR>/main.cpp:-1
#include <DIR>/a.h:-1

int foo() { return 42; }
#include <DIR>/main.cpp:0
int main() { return foo(); }
