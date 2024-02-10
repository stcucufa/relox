TARGET =	hamt-test
OBJECTS =	../hamt.o ../object.o ../value.o main.o
CFLAGS =	-Wall -pedantic -g -DDEBUG
LDFLAGS =	-lm

check:	$(TARGET)
	./$(TARGET) && echo OK

$(TARGET):	$(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:	clean
clean:
	rm -f $(TARGET) $(OBJECTS)
