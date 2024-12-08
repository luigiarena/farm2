CC = gcc
CFLAGS += -std=c99 -Wall -pedantic -g -pthread -Iheader

SOURCES = ./src
HEADERS = ./headers
TESTDIR = ./testdir
SPAZIO = @echo -n "    "

.PHONY = all test cleanall cleantest

all: farm test

farm: $(SOURCES)/*.c
	$(CC) $(CFLAGS) $^ -o $@

generafile: generafile.c 
	$(CC) $(CFLAGS) $^ -o $@

test: generafile
	./test.sh

cleanall: cleantest
	@echo ">>> Rimozione di tutti i file di oggetto, eseguibili e socket"
	$(SPAZIO)
	-rm -f farm
	$(SPAZIO)
	-rm -f generafile
	$(SPAZIO)
	-rm -f farm2.sck 
	$(SPAZIO)
	-rm -f $(SOURCES)/*.o
	$(SPAZIO)
	-rm -f *.o
	@echo -n ""

cleantest: 
	@echo ">>> Rimozione di tutti i file di test"
	$(SPAZIO)
	-rm -f *.dat
	$(SPAZIO)
	-rm -f *.txt
	$(SPAZIO)
	-rm -f -r $(TESTDIR)
	@echo -n ""
	