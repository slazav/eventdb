CFG_FILE ?= ./config.json

# Include ../Makefile.def with MCCME setting if it exists
# on mccme I have local installation of jansson and db in ../../SRC
-include ../Makefile.def
ifndef MCCME
  CPPFLAGS = $(shell pkg-config --cflags jansson)
  LDLIBS = $(shell pkg-config --libs jansson)
  LDLIBS += -lcrypto -ldb
else
  CPPFLAGS=-I../../SRC
  LDLIBS=../../SRC/libjansson.a ../../SRC/libdb.a -lcrypto -lpthread
  CXX=g++
endif

CPPFLAGS += -DCFG_FILE=\"$(CFG_FILE)\"

all: eventdb dump_db

eventdb : eventdb.cpp err.cpp log.cpp login.cpp actions.cpp jsondb/jsondb.cpp

dump_db : dump_db.cpp err.cpp log.cpp login.cpp actions.cpp jsondb/jsondb.cpp

clean:
	rm -f eventdb

