CC = gcc
CFLAGS = -Wall -Werror -g
LDFLAGS = -lncursesw

all: nibbler

nibbler: *.c *.h
	$(CC) $(CFLAGS) -o $@ *.c $(LDFLAGS)

clean:
	rm -f nibbler
