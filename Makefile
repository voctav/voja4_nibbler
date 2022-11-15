CC = gcc
CFLAGS = -O3 -Wall -Werror
DEBUG_CFLAGS = -g -fsanitize=address -fsanitize=leak
LDFLAGS = -lncursesw

all: nibbler

nibbler: *.c *.h
	$(CC) $(CFLAGS) -o $@ *.c $(LDFLAGS)

nibbler_debug: *.c *.h
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $@ *.c $(LDFLAGS)

clean:
	rm -f nibbler nibbler_debug *.exe
