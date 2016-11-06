CFLAGS = $(shell pkg-config --cflags jansson)
LDLIBS = $(shell pkg-config --libs jansson)
LDLIBS += -ldb

all: test1

test_jsondb: test_jsondb.cpp jsondb.cpp jsondb.h Makefile
	$(CXX) -o $@ -g -O0 -Wall $(CFLAGS) jsondb.cpp test_jsondb.cpp $(LDLIBS)

test1: test_jsondb
	./test_jsondb

clean:
	rm -f test_jsondb
