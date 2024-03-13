TARGET =	relox
OBJECTS =	array.o compiler.o hamt.o lexer.o main.o value.o vm.o
OPT_FLAGS =	-g -DDEBUG
CFLAGS =	-Wall -pedantic $(OPT_FLAGS)
LDFLAGS =	-lm

$(TARGET):	$(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:	check clean

check:
	cd hamt && $(MAKE) check

clean:
	rm -f $(TARGET) $(OBJECTS)
	cd hamt && $(MAKE) clean
