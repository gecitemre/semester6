SRCS = hw2.cpp matrix.hpp hw2_output.c hw2_output.h
TARFILE = hw2.tar.gz

all: hw2

hw2: $(SRCS)
	g++ -pthread $(SRCS) -o hw2

tar: $(TARFILE)

hw2.tar.gz: $(SRCS) Makefile
	tar -czvf $(TARFILE) hw2.cpp matrix.hpp Makefile

test: tar
	tar -xf $(TARFILE)
	make
	./hw2 < input_example.txt

clear:
	rm -f $(TARFILE) hw2