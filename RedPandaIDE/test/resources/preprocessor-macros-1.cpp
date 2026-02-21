// Make function factory and use it
#define FUNCTION(name,a) int fun_##name() { return a; }

FUNCTION(abcd, 12)
FUNCTION(fff, 2)
FUNCTION(qqq, 23)

#undef FUNCTION
#define FUNCTION 34
#define OUTPUT(a) std::cout << "output: " #a << '\n'

// Using a macro in the definition of a later macro
#define WORD "Hello "
#define OUTER(...) WORD #__VA_ARGS__

int main()
{
	std::cout << "abcd: " << fun_abcd() << '\n';
	std::cout << "fff: " << fun_fff() << '\n';
	std::cout << "qqq: " << fun_qqq() << '\n';
	
	std::cout << FUNCTION << '\n';
	OUTPUT(million); //note the lack of quotes
	
	std::cout << OUTER(World) << '\n';
	std::cout << OUTER(WORD World) << '\n';
}
