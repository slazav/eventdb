LDLIBS=-ldb -lcrypto
#CFLAGS=-g

eventdb:  dbs.o user.o event.o log.o link.o actions.o eventdb.o

dbs.o user.o event.o geo.o: dbs.h
actions.o: actions.h
event.o link.o geo.o: event.h

clean:
	rm -f -- *.o eventdb

