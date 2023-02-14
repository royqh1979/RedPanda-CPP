	.global main

	.text:
main:
	# the x64 calling convention requires you to allocate 32 bytes of shadow space before each call
	# https://stackoverflow.com/questions/30190132/what-is-the-shadow-space-in-x64-assembly/
	sub 	$32, %rsp           # allocate shadow space
	leaq	fmt(%rip), %rcx # first parameter 
	leaq	msg(%rip), %rdx # second parameter
	call 	printf
	add 	$32, %rsp           # remove shadow space

	xor 	%eax,%eax	        # set 0 as exit code 
	ret
	

msg:
	.asciz	"Hello world\n"  # asciz puts a 0 byte at the end
fmt:
	.asciz	"%s"	 # asciz puts a 0 byte at the end