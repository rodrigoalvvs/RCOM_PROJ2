CC = gcc
CFLAGS = -Wall -g

DEBUG ?= 0

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
endif

SRC = src/
INCLUDE = include/
BIN = bin/

.PHONY: downloader
downloader: $(SRC)/clientFTP.c
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -o bin/download $^

.PHONY: clean
clean:
	rm -rf bin/*