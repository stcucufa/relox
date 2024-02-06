OBJECTS =	array.o compiler.o lexer.o main.o value.o vm.o
CFLAGS =	-Wall -pedantic -g -DDEBUG
LDFLAGS =	-lm

relox:	$(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:	clean
clean:
	rm -f relox $(OBJECTS)
