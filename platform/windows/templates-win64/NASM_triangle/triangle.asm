          global    main
		  extern printf
section   .text
main:
	sub rsp,32
	lea       rdx, [rel output]             ; rdx holds address of next byte to write
	mov       r8, 1                   ; initial line length
	mov       r9, 0                   ; number of stars written on line so far
line:
	mov       byte [rdx], '*'         ; write single star
	inc       rdx                     ; advance pointer to next cell to write
	inc       r9                      ; "count" number so far on line
	cmp       r9, r8                  ; did we reach the number of stars for this line?
	jne       line                    ; not yet, keep writing on this line
lineDone:
	mov       byte [rdx], 10          ; write a new line char
	inc       rdx                     ; and move pointer to where next char goes
	inc       r8                      ; next line will be one char longer
	mov       r9, 0                   ; reset count of stars written on this line
	cmp       r8, maxlines            ; wait, did we already finish the last line?
	jng       line                    ; if not, begin writing this line
done:
    mov		byte [rdx], 0
	lea		rdx, [rel fmt]          ; first parameter
	lea		rcx, [rel output]       ; second parameter
	call	printf                    ; invoke operating system to do the write
	sub		rsp,32
    xor		eax,eax
	ret
	
fmt:	db "%s",0

section   .bss
maxlines  equ       8
dataSize  equ       44
output:   resb      dataSize

