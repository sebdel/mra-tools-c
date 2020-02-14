TARGET=mra
SRC = ./src
SRCS = $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/*/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
CC=gcc
LIBS = -lz
CFLAGS = -O2 -DHAVE_ZLIB -Isrc/junzip -Isrc/sxmlc -Isrc/md5
__sha1 := $(shell echo "char *sha1 = \"$(shell git rev-parse HEAD)\";" > src/sha1.c);

$(info Building $(TARGET) from $(SRCS)...)

all: clean $(TARGET)
	
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

check:	all
	./test.sh

clean:
	find . -type f -name '*.o' -exec rm {} +
	rm -f mra