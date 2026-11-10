all: libraries.h
	gcc postoffice.c -pthread -o postOffice.out

clean:
	rm -f postOffice.out