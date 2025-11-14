	.text
	.globl main
main:
    # Microsoft X86_64 stack usage: 
	# - stack should be maintained 16-byte aligned 
	# see https://learn.microsoft.com/en-us/cpp/build/stack-usage
	
	# Microsoft X86_64 Calling convention:
	# - The first four integer or pointer parameters are passed in the rcx, rdx, r8, and r9 registers.
	# - The first four floating-point parameters are passed in the first four SSE registers, xmm0-xmm3.
	# - The caller reserves space on the stack for arguments passed in registers(32 bytes). Debuggers can use it to store debug infos.
	# - Any additional arguments are passed on the stack.
	# - An integer or pointer return value is returned in the rax register, while a floating-point return value is returned in xmm0.
	# see https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/x64-architecture
	
	sub 	$32, %rsp           # reserve stack space for the call
	leaq	fmt(%rip), %rcx		# first parameter 
	leaq	msg(%rip), %rdx		# second parameter
	call 	printf
	add 	$32, %rsp           # restore stack space

	xor 	%eax,%eax	        # set 0 as exit code 
	ret
	

msg:
	.asciz	"Hello world\n"  # asciz puts a 0 byte at the end
fmt:
	.asciz	"%s"	 # asciz puts a 0 byte at the end