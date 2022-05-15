TARGET = ./main.out
HDRS = \
	   project/include

SRCS = \
	   project/src/main.c \
	   project/src/frame_utils.c \
	   project/src/timestamps.c

.PHONY: all check build rebuild clean

all: clean check build

check:
	./run_linters.sh

build: $(TARGET)

rebuild: clean build

$(TARGET): $(SRCS)
	$(CC) -I $(HDRS) $(SRCS) -lncurses -o $(TARGET)

clean:
	rm -f $(TARGET) Logs.txt StartIndicator
