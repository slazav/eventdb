#JANSSON_CFLAGS := $(shell pkg-config --cflags jansson)
#JANSSON_LIBS := $(shell pkg-config --libs jansson)

LDLIBS=../jansson/src/.libs/libjansson.a -ldb
CFLAGS=-I../jansson/src 
#-std=c++11

all: test1

test_jsondb: test_jsondb.cpp jsondb.cpp jsondb.h Makefile
	$(CXX) -o $@ -g -O0 -Wall $(CFLAGS) jsondb.cpp test_jsondb.cpp $(LDLIBS)

test1: test_jsondb
	./test_jsondb

clean:
	rm -f test_jsondb
