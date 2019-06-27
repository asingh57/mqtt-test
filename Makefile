.PHONY: async sync

# Compiler
CC=gcc
LIBS=-lpaho-mqtt3c -lpaho-mqtt3a -lpaho-mqtt3as
CFLAGS=-g -Wall
BENCHMARKER=benchmark.c
SOURCE_SYNC=
SOURCE_ASYNC=

BIN_DIR=./bin

all: async sync

async: async_client_pub.c async_client_sub.c
	$(CC) -o $(BIN_DIR)/async_client_pub $(CFLAGS) async_client_pub.c $(BENCHMARKER) $(LIBS)
	$(CC) -o $(BIN_DIR)/async_client_sub $(CFLAGS) async_client_sub.c $(BENCHMARKER) $(LIBS)

sync: client_sync_pub.c client_async_pub.c client_async_sub.c $(BENCHMARKER)
	$(CC) -o $(BIN_DIR)/client_sync_pub $(CFLAGS) client_sync_pub.c $(BENCHMARKER) $(LIBS)
	$(CC) -o $(BIN_DIR)/client_async_pub $(CFLAGS) client_async_pub.c $(BENCHMARKER) $(LIBS)
	$(CC) -o $(BIN_DIR)/client_async_sub $(CFLAGS) client_async_sub.c $(BENCHMARKER) $(LIBS)

clean:
	rm $(BIN_DIR)/*
