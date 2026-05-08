#define FUNCTION(name,a) int fun_##name() { return a; }

FUNCTION
(abcd, 12
)
int x;