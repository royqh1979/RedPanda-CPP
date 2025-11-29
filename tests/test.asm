	global main
	extern puts
section .text
main:
	sub rsp, 20h
	mov rcx, [message]
	call puts
	add rsp, 20h
	ret 
message: db "Hello,world!",0
