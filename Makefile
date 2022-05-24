CC = gcc 
CFLAGS= -std=gnu99 -Wall -Wpedantic -Wextra -Wvla

TARGET = ./main.out
HDRS = \
	   project/include

SRCS = \
	   project/src/main.c \
	   project/src/frame_utils.c \
	   project/src/timestamps.c \
	   project/src/argparsing.c \
	   project/src/videostream.c

.PHONY: all check build rebuild clean

all: clean check build

check:
	./run_linters.sh

build: $(TARGET)

rebuild: clean build

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -I $(HDRS) $(SRCS) -lncurses -o $(TARGET) -g

clean:
	rm -f $(TARGET) Logs.txt StartIndicator

