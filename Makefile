LDLIBS=-ldb -lcrypto
#CFLAGS=-g

eventdb:  dbs.o user.o event.o log.o actions.o eventdb.o

dbs.o user.o event.o: dbs.h
user.o actions.o: user.h
event.o actions.o: event.h
actions.o: actions.h

clean:
	rm -f -- *.o eventdb

