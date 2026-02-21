#include <stdio.h>
 
#define PUTS puts



/* Line splicing is in phase 2 while macros
 * are tokenized in phase 3 and expanded in phase 4,
 * so the above is equivalent to #define PUTS puts
 */
 
int main(void)
{
 /* Use line splicing to call puts */ PUTS("Output ends here\0Not printed" /* After line splicing, the remaining backslash



               * escapes the 0, ending the string early.
               */
);
}