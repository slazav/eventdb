#JANSSON_CFLAGS := $(shell pkg-config --cflags jansson)
#JANSSON_LIBS := $(shell pkg-config --libs jansson)

LDLIBS=../jansson/src/.libs/libjansson.a
CFLAGS=-I../jansson/src

all: test1

test_jsonxx: test_jsonxx.cpp jsonxx.h Makefile
	$(CXX) -o $@ -g -O0 -Wall $(CFLAGS) test_jsonxx.cpp $(LDLIBS)

test1: test_jsonxx
	./test_jsonxx

clean:
	rm -f test_jsonxx
