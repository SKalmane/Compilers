CC = gcc
CFLAGS += -g -Wall -Wextra -pedantic

LEX = flex
LFLAGS += --header-file=scanner.h

YACC = bison
YFLAGS += -d -v

%.c : %.y
	$(YACC) $(YFLAGS) -o $@ $<

LDLIBS += -lfl -ly
LDFLAGS =

EXECS = compiler

# House-keeping build targets.

all : $(EXECS)

clean :
	rm -f $(EXECS) *.o scanner.c scanner.h parser.c parser.h

scanner.h : scanner.c

parser.h : parser.c

node.o : node.c node.h

parser.o : parser.c node.h

scanner.o : scanner.c parser.h node.h

symbol.o : symbol.c symbol.h node.h

type.o : type.c type.h symbol.h node.h

ir.o : ir.c ir.h type.h symbol.h node.h

basic_blocks.o : basic_blocks.c basic_blocks.h ir.h

mips.o : mips.c mips.h ir.h type.h symbol.h node.h

compiler.o : compiler.c mips.h ir.h type.h symbol.h node.h parser.h scanner.h basic_blocks.h
compiler: compiler.o parser.o scanner.o node.o symbol.o type.o ir.o mips.o basic_blocks.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS) $(CFLAGS)
