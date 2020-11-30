CC = /usr/bin/gcc
AS = /usr/bin/as
LD = /usr/bin/ld
CFlags =   -Wall -Wextra -pedantic -std=c99
editor: editor.c
	$(CC) editor.c -o zr $(CFlags)

test:  test.c
	$(CC) test.c -o test $(CFlags)

masm : editor.c 
	$(CC) -S -masm=intel editor.c -o intel_editor.s
	$(CC) -S -masm=att editor.c -o att_editor.s
steps: masm 
	$(CC) -S -masm=intel editor.c -o editor.s
	$(AS) editor.s -o editor.o
	#$(LD) editor.o 

all: editor test masm
	
