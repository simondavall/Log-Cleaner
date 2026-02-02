CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c11

.PHONY: test1, test2, test3
test1: log-cleaner
	./log-cleaner ~/Projects/C/Log-Cleaner/sample.log ./log-cleaner-config.json

test2: log-cleaner
	./log-cleaner --retain ~/Projects/C/Log-Cleaner/sample.log ./log-cleaner-config.json

test3: log-cleaner-memcheck
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./log-cleaner-memcheck ~/Projects/C/Log-Cleaner/sample.log ./log-cleaner-config.json
 

log-cleaner-memcheck: main.o cJSON.o
	$(CC) -g -o log-cleaner-memcheck main.o cJSON.o $(CFLAGS)

log-cleaner: main.o cJSON.o
	$(CC) -o log-cleaner main.o cJSON.o $(CFLAGS)

main.o: main.c cJSON.h
	$(CC) -c main.c $(CFLAGS)

cjson.o: cJSON.c cJSON.h
	$(CC) -c cJSON.c $(CFLAGS)
