OBJS = main.o sxmlc.o junzip.o
LIBS = -lz
FLAGS = -O2 -DHAVE_ZLIB

all: mra-tools

mra-tools: main.o sxmlc.o junzip.o
	gcc -o mra-tools $(OBJS) $(LIBS)

junzip.o: src/junzip.c
	gcc -c $(FLAGS) src/junzip.c

sxmlc.o: src/sxmlc.c
	gcc -c $(FLAGS) src/sxmlc.c

main.o: src/main.c
	gcc -c $(FLAGS) src/main.c

clean:
	rm *.o
	rm mra-tools