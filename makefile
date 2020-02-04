TARGET=mra
SRC = ./src
SRCS = $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/*/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
#CC= x86_64-w64-mingw32-gcc-5.3-posix
CC=gcc
LIBS = -lz
CFLAGS = -O2 -DHAVE_ZLIB -Isrc/junzip -Isrc/sxmlc -Isrc/md5
__sha1 := $(shell echo "char *sha1 = \"$(shell git rev-parse HEAD)\";" > src/sha1.c);

$(info Building $(TARGET) from $(SRCS)...)

all: clean $(TARGET)
	
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

check:
	./test.sh

clean:
	find . -type f -name '*.o' -exec rm {} +
	rm -f mra