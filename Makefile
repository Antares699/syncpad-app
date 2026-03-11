CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lncurses -ltinfo $(shell pkg-config --libs --cflags libgit2)

# On Windows (MSYS2), we need pdcurses instead
ifeq ($(OS),Windows_NT)
	LDFLAGS = -lpdcurses $(shell pkg-config --libs --cflags libgit2)
	TARGET = syncpad.exe
else
	TARGET = syncpad
endif

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c $(LDFLAGS) -o $(TARGET)

clean:
	rm -f syncpad syncpad.exe *.o
