all: minicron
main.o: main.c functions.h
	gcc -c main.c
functions.o: functions.c functions.h
	gcc -c functions.c
minicron: main.o functions.o
	gcc -o minicron main.o functions.o
.PHONY: clean
clean:
	rm -f *.o minicron
