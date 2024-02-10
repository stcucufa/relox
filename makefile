TARGET =	relox
OBJECTS =	array.o compiler.o hamt.o lexer.o main.o object.o value.o vm.o
CFLAGS =	-Wall -pedantic -g -DDEBUG
LDFLAGS =	-lm

$(TARGET):	$(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:	clean
clean:
	rm -f $(TARGET) $(OBJECTS)
