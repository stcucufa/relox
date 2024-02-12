TARGET =	relox
OBJECTS =	array.o compiler.o hamt.o lexer.o object.o value.o vm.o
CFLAGS =	-Wall -pedantic -g -DDEBUG
LDFLAGS =	-lm

HAMT_TEST =	hamt-test

$(TARGET):	$(OBJECTS) main.o
	$(CC) $^ $(LDFLAGS) -o $@

$(HAMT_TEST):	$(OBJECTS) $(HAMT_TEST).o
	$(CC) $^ $(LDFLAGS) -o $@

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:	check clean

check:	$(HAMT_TEST)
	./$(HAMT_TEST) && echo ok

clean:
	rm -f $(TARGET).o $(TARGET) $(HAMT_TEST).o $(HAMT_TEST) $(OBJECTS)
