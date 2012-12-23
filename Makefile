LDLIBS=-ldb -lcrypto
#CFLAGS=-g
eventdb:  dbs.o user.o actions.o eventdb.o

clean:
	rm -f -- *.o eventdb

