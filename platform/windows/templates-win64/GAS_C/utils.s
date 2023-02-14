    .global  maxofthree
    .global  add3
    .text
maxofthree:
	mov     %ecx, %eax              # result (rax) initially holds x
	cmp     %edx, %eax                # is x less than y?
	cmovl   %edx, %eax                # if so, set result to y
	cmp     %r8d, %eax                # is max(x,y) less than z?
	cmovl   %r8d, %eax                # if so, set result to z
	ret                             # the max will be in rax
add3:
	mov %ecx, %eax
	add %edx, %eax
	add %r8d, %eax
	ret
