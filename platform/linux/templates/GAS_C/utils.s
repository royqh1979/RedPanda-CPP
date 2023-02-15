    .text
    .globl  maxofthree
    .globl  add3
maxofthree:
	mov     %edi, %eax              # result (rax) initially holds x
	cmp     %esi, %eax                # is x less than y?
	cmovl   %esi, %eax                # if so, set result to y
	cmp     %edx, %eax                # is max(x,y) less than z?
	cmovl   %edx, %eax                # if so, set result to z
	ret                             # the max will be in rax
add3:
	mov %edi, %eax
	add %esi, %eax
	add %edx, %eax
	ret
