CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

all: echos echo

echos: echos.c
	$(CC) $(CFLAGS) -o echos echos.c

echo: echo.c
	$(CC) $(CFLAGS) -o echo echo.c

clean:
	rm -f echos echo *.o
	rm -rf *.dSYM