all: test

CC = g++
OPT= -ggdb -flto -Ofast -mavx
# OPT= -ggdb -flto
COPT =
CFLAGS = $(OPT) -Wall $(COPT)
LIBS = -lssl -lcrypto

test: test.cc sketch.cc zipf.c hashutil.c count_min_sketch.cc \
	misra_gries.cc misra_gries.h count_sketch.cc count_sketch.h
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f test test.o
