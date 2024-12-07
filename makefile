CC = gcc
CFLAGS += -std=c99 -Wall -pedantic -g -pthread -Iheader

TARGETS = generafile farm

SOURCES = $./src
HEADERS = $./headers
TEST = $./test

.PHONY = all test cleanall cleantest

generafile: generafile.c 
		$(CC) $(CFLAGS) -o $@ $^

test: generafile
	  ./test.sh

cleanall: cleantest
	-rm farm2
	-rm generafile
	-rm $(SOURCES)/*.o

cleantest:
	-rm farm2.sck
	-rm -f *.dat
	-rm -f *.txt
	-rm -f -r test
	