    global  maxofthree
    global  add3
section .text
maxofthree:
	mov     eax, ecx              # result (rax) initially holds x
	cmp     eax, edx                # is x less than y?
	cmovl   eax, edx                # if so, set result to y
	cmp     eax, r8d                # is max(x,y) less than z?
	cmovl   eax, r8d                # if so, set result to z
	ret                             # the max will be in rax
add3:
	mov eax, ecx
	add eax, edx
	add eax, r8d
	ret
