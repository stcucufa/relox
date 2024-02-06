OBJECTS =	array.o main.o vm.o
CFLAGS =	-Wall -pedantic -g -DDEBUG

relox:	$(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

%.o:	%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:	clean
clean:
	rm -f relox $(OBJECTS)
