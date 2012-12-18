LDLIBS=-ldb -lcrypto
eventdb: actions.o user.o eventdb.o dbs.o

clean:
	rm -f -- *.o eventdb

