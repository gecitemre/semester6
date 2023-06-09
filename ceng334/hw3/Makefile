CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -DTIMESTAMP=0x12345678
INCLUDES = -I./include

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

TARGET = bin/je2fs

.PHONY: all clean debug

all: $(TARGET)

debug: $(TARGET) $(IMAGE)
	gdb -x gdbinit ./$<

$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(OBJS): $(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(TARGET) $(OBJS) $(OBJDIR)

