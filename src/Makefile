CC=gcc
CFLAGS=-Wall -Werror

all: tiny adder tinyproxy

tiny: tiny.o csapp.o 
	$(CC) -o $@ $^ -lpthread

adder: adder.o csapp.o 
	$(CC) -o $@ $^ -lpthread

csapp.o : csapp.c csapp.h 
	$(CC) $(CFLAGS) -c -o $@ $<
	
tinyproxy: tinyproxy.o csapp.o signal.o filtrage.o journal.o tinyproxy.h
	$(CC) -o $@ $^ -lpthread

adder.o: adder.c csapp.h
	$(CC) $(CFLAGS) -c -o $@ $<

signal.o : signal.c csapp.h tinyproxy.h
	$(CC) $(CFLAGS) -c -o $@ $<

journal.o : journal.c tinyproxy.h
	$(CC) $(CFLAGS) -c -o $@ $<

filtrage.o : filtrage.c tinyproxy.h
	$(CC) $(CFLAGS) -c -o $@ $<
	
tiny.o: tiny.c csapp.h 
	$(CC) $(CFLAGS) -c -o $@ $<
	
tinyproxy.o: tinyproxy.c csapp.h tinyproxy.h
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm *.o tinyproxy tiny adder



