
#define FUNCTION(name,a) int fun_ ##name() { return a; }

int fun_abcd() { return 12; }
int fun_fff() { return 2; }
int fun_qqq() { return 23; }

#undef FUNCTION
#define FUNCTION 34
#define OUTPUT(a) std::cout << "output: " #a << '\n'


#define WORD "Hello "
#define OUTER(...) WORD #__VA_ARGS__

int main()
{
	std::cout << "abcd: " << fun_abcd() << '\n';
	std::cout << "fff: " << fun_fff() << '\n';
	std::cout << "qqq: " << fun_qqq() << '\n';
	
	std::cout << 34 << '\n';
	std::cout << "output: " "million" << '\n';
	
	std::cout << "Hello " "World" << '\n';
	std::cout << "Hello " "WORD World" << '\n';
}
