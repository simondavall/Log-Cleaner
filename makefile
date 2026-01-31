CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c11

.PHONY: test
test: log-cleaner
	./log-cleaner ~/Projects/C/Log-Cleaner/sample.log ./log-cleaner-config.json

log-cleaner: main.o cJSON.o
	$(CC) -o log-cleaner main.o cJSON.o $(CFLAGS)

main.o: main.c cJSON.h
	$(CC) -c main.c $(CFLAGS)

cjson.o: cJSON.c cJSON.h
	$(CC) -c cJSON.c $(CFLAGS)
