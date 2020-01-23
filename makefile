TARGET=mra
SRC = ./src
SRCS = $(wildcard $(SRC)/*.c) $(wildcard $(SRC)/*/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
CC=gcc
LIBS = -lz
CFLAGS = -O2 -DHAVE_ZLIB -Isrc/junzip -Isrc/sxmlc -Isrc/md5

$(info Building $(TARGET) from $(SRCS)...)

all: clean $(TARGET)
	
$(TARGET): $(OBJS)
	gcc -o $(TARGET) $(OBJS) $(LIBS)

clean:
	find . -type f -name '*.o' -exec rm {} +
	rm -f mra