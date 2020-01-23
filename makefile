OBJS = main.o unzip.o mra.o sxmlc.o junzip.o md5.o
LIBS = -lz
FLAGS = -O2 -DHAVE_ZLIB -Isrc/junzip -Isrc/sxmlc -Isrc/md5

all: mra-tools

mra-tools: $(OBJS)
	gcc -o mra-tools $(OBJS) $(LIBS)

junzip.o: src/junzip/junzip.c
	gcc -c $(FLAGS) src/junzip/junzip.c

sxmlc.o: src/sxmlc/sxmlc.c
	gcc -c $(FLAGS) src/sxmlc/sxmlc.c

md5.o: src/md5/md5.c
	gcc -c $(FLAGS) src/md5/md5.c

unzip.o: src/unzip.c
	gcc -c $(FLAGS) src/unzip.c

mra.o: src/mra.c
	gcc -c $(FLAGS) src/mra.c

main.o: src/main.c
	gcc -c $(FLAGS) src/main.c

clean:
	rm *.o
	rm mra-tools