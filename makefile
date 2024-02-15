TARGET =	relox
OBJECTS =	array.o compiler.o hamt.o hash-table.o lexer.o main.o object.o value.o vm.o
CFLAGS =	-Wall -pedantic -g -DDEBUG
LDFLAGS =	-lm

HAMT_TEST_TARGET =	hamt-test
HAMT_TEST_OBJECTS =	hamt.o hamt-test.o object.o value.o

HASH_TEST_TARGET =	hash-test
HASH_TEST_OBJECTS =	hash-table.o hash-test.o object.o value.o

$(TARGET):	$(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

$(HAMT_TEST_TARGET):	$(HAMT_TEST_OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

$(HASH_TEST_TARGET):	$(HASH_TEST_OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:	check clean

check:	$(HAMT_TEST_TARGET)
	./$(HAMT_TEST_TARGET) && echo ok

clean:
	rm -f $(TARGET) $(OBJECTS)
	rm -f $(HAMT_TEST_TARGET) $(HAMT_TEST_OBJECTS)
	rm -f $(HASH_TEST_TARGET) $(HASH_TEST_OBJECTS)
