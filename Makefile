CC = gcc
CFLAGS = -Wall

SRC = src/
INCLUDE = include/
BIN = bin/

.PHONY: downloader
downloader: $(SRC)/clientFTP.c
	$(CC) $(CFLAGS) -o bin/download $^

.PHONY: clean
clean:
	rm -rf bin/*