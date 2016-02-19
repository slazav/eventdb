
LDLIBS=./jansson-2.7/src/.libs/libjansson.a -lcrypto -ldb
LDFLAGS=-L/usr/local/lib/db42 
CPPFLAGS=-I./jansson-2.7/src -I/usr/local/include/db42 -g

all: eventdb

eventdb: eventdb.cpp err.cpp json.cpp jdb.cpp login.cpp actions.cpp