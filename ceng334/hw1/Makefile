SRCS = bgame.cpp logging.c logging.h message.c message.h
TARFILE = e2521581.tar.gz

all: bgame

bgame: $(SRCS)
	g++ -lpthread $(SRCS) -o bgame

tar: $(SRCS) Makefile
	tar -czvf $(TARFILE) bgame.cpp Makefile

test1: tar
	tar -xf $(TARFILE)
	make
	./bgame < testcases/input1.txt

test2: tar
	tar -xf $(TARFILE)
	make
	./bgame < testcases/input2.txt

test3: tar
	tar -xf $(TARFILE)
	make
	./bgame < testcases/input3.txt

test4: tar
	tar -xf $(TARFILE)
	make
	./bgame < testcases/input4.txt

tests:
	make test1 test2 test3 test4

clear:
	rm -f $(TARFILE) bgame message.h.gch logging.h.gch