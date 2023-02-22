CC = gcc
CFLAGS = -ansi -Wall -g -O0 -Wwrite-strings -Wshadow -pedantic-errors \
		-fstack-protector-all -Wextra
D8SH = d8sh

.PHONY: all clean

all: $(D8SH)

lexer.o: lexer.c lexer.h parser.tab.h
	$(CC) $(CFLAGS) -c lexer.c

parser.tab.o: parser.tab.c parser.tab.h command.h
	$(CC) $(CFLAGS) -c parser.tab.c

executor.o: executor.c executor.h command.h
	$(CC) $(CFLAGS) -c executor.c

d8sh.o: d8sh.c executor.h lexer.h
	$(CC) $(CFLAGS) -c d8sh.c

d8sh: parser.tab.o lexer.o executor.o d8sh.o
	$(CC) -o d8sh parser.tab.o lexer.o executor.o d8sh.o -lreadline

clean: 
	@rm -f *.o