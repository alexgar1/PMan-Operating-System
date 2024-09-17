.phony all:
all: pman

pman: main.c linkedls.c
	gcc -Wall main.c linkedls.c -o pman

.PHONY clean:
clean:
	-rm -rf *.o *.exe

