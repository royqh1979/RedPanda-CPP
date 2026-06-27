//not handled: mutable, constexpr, thread_local, register, volatile

//normal var
int a1;
const int a2;
static const int a3;
extern const int a4;

static const int a5[4];
static const int *a6;
static const int &a7;
static const int **a8;
static const int a9,*a10,&a11;


unsigned int b1;
const unsigned int b2;
static const unsigned int b3;
extern const unsigned int b4;
static unsigned int b5[4],*b6,* const b7=&b1,**b8;

std::string s1;
const std::string s2="test";
static const std::string s3{"test"};
static const std::string s4("test");
static const std::string s5[4];
static const std::string *s6;
static const std::string &s7;
static const ::std::string &s8;

static const struct FILE *f1,f2;

//function pointer
int (*ff1)();
static const int (*ff2)();
static const std::string (*ff3)();
static const std::string (&ff4)();
static const std::string (&& ff5)();
int* (*ff6)();

//pointer array
int (*aa1)[5];
static const std::string (*aa2)[5];


