TMPFILES = lex.c parse.c
MODULES = main parse output
OBJECTS = $(MODULES:=.o)
CC = gcc -g -Wall
LINK = $(CC)
LIBS = -lreadline

shell: $(OBJECTS)
	$(LINK) $(OBJECTS) -o $@ $(LIBS)

# Compiling:
%.o: %.c
	$(CC) -c $<

# parsers

parse.o: parse.c lex.c
parse.c: parse.y global.h
	bison parse.y -o $@
lex.c: lex.l
	flex -o$@ lex.l

# clean

clean: 
	rm -f shell $(OBJECTS) $(TMPFILES) core* *.o
