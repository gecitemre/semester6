CXX_SOURCES = hw2.cpp matrix.cpp
C_SOURCES = hw2_output.c
CXX_OBJECTS = $(CXX_SOURCES:.cpp=.o)
C_OBJECTS = $(C_SOURCES:.c=.o)
OBJECTS = $(CXX_OBJECTS) $(C_OBJECTS)
HEADERS = matrix.hpp hw2_output.h
TARGET = hw2
TARFILE = $(TARGET).tar.gz

all: $(TARGET)

$(TARGET): $(OBJECTS)
	g++ -o $(TARGET) $(OBJECTS) -lpthread

%.o: %.cpp $(HEADERS)
	g++ -c $< -o $@

%.o: %.c $(HEADERS)
	gcc -c $< -o $@

tar: $(TARFILE)

SUBMISSION = hw2.cpp matrix.cpp matrix.hpp Makefile
$(TARFILE): $(SUBMISSION)
	tar -czf $(TARFILE) $(SUBMISSION)

test: tar
	tar -xf $(TARFILE)
	make
	./$(TARGET) < input_example.txt

clean:
	rm -f $(TARFILE) $(OBJECTS) $(TARGET)