CC = gcc
CFLAGS = -Wall -O2 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -I../headers
LIBS = -lfuse

all: pifs pifs_program

pifs:
	$(CC) $(CFLAGS) code/pifs.c code/directory.c code/inode.c code/path.c code/storage.c -o pifs $(LIBS)

pifs_program:
	$(CC) $(CFLAGS) code/pifs_program.c code/inode.c code/directory.c code/path.c code/storage.c -o pifs_program

clean:
	rm -f pifs pifs_program
