SRCS = bgame.cpp logging.c logging.h message.c message.h
TARFILE = e2521581.tar.gz

bgame: $(SRCS)
	g++ $(SRCS) -o bgame

tar: $(SRCS) Makefile
	tar -cf $(TARFILE) bgame.cpp Makefile

test: tar bgame
	tar -xf $(TARFILE)
	./bgame