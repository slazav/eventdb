
LDLIBS=./jansson/src/.libs/libjansson.a -lcrypto -ldb
LDFLAGS=-L/usr/local/lib/db42 
CPPFLAGS=-I./jansson/src -I/usr/local/include/db42 -g

all: eventdb

eventdb: eventdb.cpp err.cpp log.cpp login.cpp actions.cpp jsondb/jsondb.cpp

clean:
	rm -f eventdb