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

SOURCES= demo.c ugi.c SDL/sdlmain.c SDL/ugiSDL.c other/bmp.c
OBJECTS=$(SOURCES:.c=.o)

all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

ugi.o: ugi.c ugi.h
demo.o: demo.c ugi.h other/bmp.h SDL/ugiSDL.h SDL/sdlmain.h other/bold.xbm
SDL/ugiSDL.o: SDL/ugiSDL.c ugi.h SDL/sdlmain.h other/bmp.h
SDL/sdlmain.o: SDL/sdlmain.c ugi.h other/bmp.h SDL/ugiSDL.h SDL/sdlmain.h
other/bmp.o: other/bmp.c other/bmp.h

.PHONY : clean

clean:
	-rm -f $(EXECUTABLE)
	-rm -f *.o SDL/*.o other/*.o
