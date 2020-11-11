CC=gcc
CFLAGS=-c -Wall `sdl2-config --cflags`
LDFLAGS=-lm `sdl2-config --libs`

EXECUTABLE=ugi

# Detect operating system:
# More info: http://stackoverflow.com/q/714100
ifeq ($(OS),Windows_NT)
  EXECUTABLE:=$(EXECUTABLE).exe
  LDFLAGS += -mwindows
endif

ifeq ($(BUILD),debug)
  # Debug
  CFLAGS += -O0 -g
  LDFLAGS +=
else
  # Release mode
  CFLAGS += -O2 -DNDEBUG
  LDFLAGS += -s
endif

SOURCES= main.c bmp.c ugi.c
OBJECTS=$(SOURCES:.c=.o)

all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

main.o : main.c bmp.h
bmp.o : bmp.c bmp.h

.PHONY : clean

clean:
	-rm -f $(EXECUTABLE)
	-rm -f *.o
