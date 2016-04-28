
CFG_FILE ?= ./config.json
LDLIBS=./jansson/src/.libs/libjansson.a -lcrypto -ldb
LDFLAGS=-L/usr/local/lib/db42 
CPPFLAGS=-I./jansson/src -I/usr/local/include/db42 -g -DCFG_FILE=\"$(CFG_FILE)\"

all: eventdb dump_db

eventdb : eventdb.cpp err.cpp log.cpp login.cpp actions.cpp jsondb/jsondb.cpp

dump_db : dump_db.cpp err.cpp log.cpp login.cpp actions.cpp jsondb/jsondb.cpp

clean:
	rm -f eventdb