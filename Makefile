CC=g++
CFLAGS=-Wall -Werror -O3 -std=c++11 -g -Wl,--no-as-needed -ldl -rdynamic -fstack-protector-all

# defaults to VG_N_THREADS -- calling with N_THREADS > VG_N_THREADS means lots of error messages thrown in valgrind
N_THREADS?=450
CFLAGS+=-D N_THREADS=$(N_THREADS)

INCLUDE=-Iinclude/

# include statements necessary to link all the individual libraries
INCLUDE_LIBS=-Iexternal/catch/include/ -Iexternal/exceptionpp/include/ -Iexternal/exceptionpp/include/libs/stacktrace/

LIBS=-pthread

SOURCES+=src/*cc tests/*cc libs/*/*cc
OBJECTS=$(SOURCES:.cc=.o)

EXECUTABLE=msgpp.app

.PHONY: all clean test

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@$(CC) $(CFLAGS) $(INCLUDE_LIBS) $(INCLUDE) $(OBJECTS) -o $@ $(LIBS)

test: clean $(EXECUTABLE)
	@ulimit -c unlimited && ./$(EXECUTABLE) | tee results.log

clean:
	@rm -f $(EXECUTABLE) *.o *.log core
