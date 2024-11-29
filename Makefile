CC = gcc
CFLAGS = -Wall

SRC = src/
INCLUDE = include/
BIN = bin/

.PHONY: downloader
downloader: $(SRC)/clientFTP.c
	$(CC) $(CFLAGS) -o download $^

.PHONY: clean
clean:
	rm -rf download