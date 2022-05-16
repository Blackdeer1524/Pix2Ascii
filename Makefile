TARGET = ./main.out
HDRS = \
	   project/include

SRCS = \
	   project/src/main.c \
	   project/src/frame_utils.c \
	   project/src/timestamps.c

.PHONY: all check build rebuild clean run_test

all: clean check build

check:
	./run_linters.sh

build: $(TARGET)

rebuild: clean build

$(TARGET): $(SRCS)
	$(CC) -I $(HDRS) $(SRCS) -lncurses -o $(TARGET)

run_test: build
	./main.out -f test.mp4

clean:
	rm -f $(TARGET) Logs.txt StartIndicator
