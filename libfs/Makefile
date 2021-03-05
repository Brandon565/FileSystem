# Target library
lib := libfs.a

all: $(lib)
-include disk.d fs.d

libfs.a: disk.o fs.o
	ar rcs libfs.a disk.o fs.o

disk.o: disk.c
	gcc -Wall -Werror -Wextra -c disk.c -o disk.o

fs.o: fs.c
	gcc -Wall -Werror -Wextra -c fs.c -o fs.o

clean:
	rm -f libfs.a disk.o fs.o
