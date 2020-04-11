all: mode1 mode2

mode1: mode1.c
	gcc -o mode1 mode1.c

mode2: mode2.c
	gcc -o mode2 mode2.c

clean:
	rm mode1
	rm mode2
