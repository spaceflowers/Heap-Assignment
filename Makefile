CC=       	gcc
CFLAGS= 	-g -gdwarf-2 -std=gnu99 -Wall
LDFLAGS=
LIBRARIES=      lib/libmalloc-ff.so \
		lib/libmalloc-nf.so \
		lib/libmalloc-bf.so \
		lib/libmalloc-wf.so

TESTS=		tests/test1 \
                tests/test2 \
                tests/test3 \
                tests/test4 \
		tests/test5 \
                tests/bfwf \
                tests/ffnf 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all:    $(LIBRARIES) $(TESTS)

lib/libmalloc-ff.so:     src/malloc.c
	$(CC) -shared -fPIC $(CFLAGS) -DFIT=0 -o $@ $< $(LDFLAGS)

lib/libmalloc-nf.so:     src/malloc.c
	$(CC) -shared -fPIC $(CFLAGS) -DNEXT=0 -o $@ $< $(LDFLAGS)

lib/libmalloc-bf.so:     src/malloc.c
	$(CC) -shared -fPIC $(CFLAGS) -DBEST=0 -o $@ $< $(LDFLAGS)

lib/libmalloc-wf.so:     src/malloc.c
	$(CC) -shared -fPIC $(CFLAGS) -DWORST=0 -o $@ $< $(LDFLAGS)

clean:
	rm -f $(LIBRARIES) $(TESTS)

run1:
	env LD_PRELOAD=lib/libmalloc-ff.so tests/test1 && echo "\nTest 1 run successfully" &&\
	env LD_PRELOAD=lib/libmalloc-ff.so tests/test2 && echo "\nTest 2 run successfully" &&\
	env LD_PRELOAD=lib/libmalloc-ff.so tests/test3 && echo "\nTest 3 run successfully" &&\
	env LD_PRELOAD=lib/libmalloc-ff.so tests/test4 && echo "\nTest 4 run successfully" &&\
	env LD_PRELOAD=lib/libmalloc-ff.so tests/test5 && echo "\nTest 5 run successfully" &&\
	echo "Sorry, your free trial on happiness has expired."
run2:
	echo "Note: These tests are meant to be run with splitting and coealesing disabled." &&\
	echo "\nUsing first fit" && env LD_PRELOAD=lib/libmalloc-ff.so tests/ffnf &&\
	echo "\nUsing next fit" && env LD_PRELOAD=lib/libmalloc-nf.so tests/ffnf &&\
	echo "\nUsing worst fit" && env LD_PRELOAD=lib/libmalloc-wf.so tests/bfwf &&\
	echo "\nUsing best fit" && env LD_PRELOAD=lib/libmalloc-bf.so tests/bfwf &&\
	echo "All tests conducted."

.PHONY: all clean
