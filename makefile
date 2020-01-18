OBJS = main.o sxmlc.o
LIBS = 
FLAGS = -O2
all: mra-tools

mra-tools: main.o sxmlc.o
	gcc -o mra-tools $(OBJS) $(LIBS)

sxmlc.o: src/sxmlc.c
	gcc -c $(FLAGS) src/sxmlc.c

main.o: src/main.c
	gcc -c $(FLAGS) src/main.c

clean:
	rm *.o
	rm mra-tools