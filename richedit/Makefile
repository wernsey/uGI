CC=gcc
CFLAGS=-c -Wall `sdl2-config --cflags`
LDFLAGS=-lm `sdl2-config --libs`

EXECUTABLE=richedit

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

SOURCES=test.c richedit.c ../ugi.c ../SDL/sdlmain.c ../SDL/ugiSDL.c ../other/bmp.c
OBJECTS=$(SOURCES:.c=.o)

all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

richedit.o: richedit.c richedit.h ../ugi.h
test.o: test.c ../ugi.h ../other/bmp.h ../SDL/ugiSDL.h ../SDL/sdlmain.h ../other/bold.xbm
../ugi.o: ../ugi.c ../ugi.h
../SDL/ugiSDL.o: ../SDL/ugiSDL.c ../ugi.h ../other/bmp.h
../SDL/sdlmain.o: ../SDL/sdlmain.c ../ugi.h ../SDL/ugiSDL.h ../SDL/sdlmain.h ../other/bmp.h
../other/bmp.o: ../other/bmp.c ../other/bmp.h


.PHONY : clean

clean:
	-rm -f $(EXECUTABLE)
	-rm -f *.o
