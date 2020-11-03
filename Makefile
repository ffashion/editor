CC = /usr/bin/gcc
CFlags =   -Wall -Wextra -pedantic -std=c99
editor: editor.c
	$(CC) editor.c -o editor $(CFlags)

test:  test.c
	$(CC) test.c -o test $(CFlags)

all: editor test
	
