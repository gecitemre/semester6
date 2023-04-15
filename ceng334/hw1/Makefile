SRCS = bgame.cpp logging.c logging.h message.c message.h
TARFILE = e2521581.tar.gz
INPUTFILE = input1.txt

bgame: $(SRCS)
	g++ $(SRCS) -o bgame

tar: $(SRCS) Makefile
	tar -cf $(TARFILE) bgame.cpp Makefile

test: tar
	tar -xf $(TARFILE)
	make
	./bgame < testcases/$(INPUTFILE)