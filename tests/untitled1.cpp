class AAA{
public:
	AAA(int id, const char* name);
	
	AAA test();
private:
	int mId;
	char* mName;
};

int main() {
	AAA a(1,"lala");
	
}
