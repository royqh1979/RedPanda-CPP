    global  main
    extern  printf
	default rel

	section .text
main:
    sub rsp, 20h
	lea rcx, message
	call printf
	add rsp, 20h
	
    ret
message:
    db  'ASM!', 0
