# z7cat utility makefile

APP = z7cat
CC = gcc
CFLAGS = -O3 -Wall -I.
OBJ = z7cat.o

.PHONY: all clean

all: clean $(APP)

$(APP): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@
	rm -f *.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(APP)