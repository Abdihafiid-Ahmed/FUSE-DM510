CC = gcc
CFLAGS = -Wall -O2 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -I../headers
LIBS = -lfuse
TARGETS = pifs pifs_format

PIFS_SRC = pifs.c
FORMAT_SRC = pifs_format.c


PIFS_OBJ = $(PIFS_SRC:.c=.o)
FORMAT_OBJ = $(FORMAT_SRC:.c=.o)


all: $(TARGETS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
pifs: $(PIFS_OBJ)
	$(CC) $(PIFS_OBJ) -o pifs $(LIBS)
pifs_format: $(FORMAT_OBJ)
	$(CC) $(FORMAT_OBJ) -o pifs_format


clean:
	rm -f *.o $(TARGETS)
