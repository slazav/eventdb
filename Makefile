LDLIBS=-ldb -lcrypto
#CFLAGS=-g

eventdb:  dbs.o user.o event.o log.o geo.o actions.o eventdb.o

dbs.o user.o event.o geo.o: dbs.h
user.o actions.o: user.h
actions.o: actions.h

clean:
	rm -f -- *.o eventdb

