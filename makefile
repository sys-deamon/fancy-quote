# Variables
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lncursesw

# Final Target
fancyquote: main.o
	$(CC) main.o -o $@ $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o $@

# Clean up build artifacts
clean:
	@rm -f fancyquote main.o *.c~
