CC := gcc

all:
	$(CC) -o main tools.c niggalloc.c

.PHONY: clean
clean:
	rm main