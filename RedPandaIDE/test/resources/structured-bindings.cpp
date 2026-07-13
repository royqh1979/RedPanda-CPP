struct T {
	int xxx;
	short yyy;
	double zzz;
};

T f(int x);

auto [a,b,c]=T();
auto [d,e,g]=f(5);



