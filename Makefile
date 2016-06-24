INCS=-I.
CPPFLAGS=-Wall -g -std=c++11 $(INCS)

TARGETS=emServer emClient

TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS=emServer.cpp emClient.cpp README Makefile

all: $(TARGETS)

emServer: emServer.cpp
	g++ $(CPPFLAGS) -pthread emServer.cpp -o $@
	
emClient: emClient.cpp
	g++ $(CPPFLAGS) emClient.cpp -o $@

clean:
	rm -f $(TARGETS) emServer.o emClient.o $(TARNAME) *~ *core *.o

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
