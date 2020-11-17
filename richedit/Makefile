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

SOURCES=test.c richedit.c ../sdlmain.c ../bmp.c ../ugi.c ../ugiSDL.c
OBJECTS=$(SOURCES:.c=.o)

all: $(EXECUTABLE)

debug:
	make BUILD=debug

$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

../bmp.o: ../bmp.c ../bmp.h
../sdlmain.o: ../sdlmain.c ../ugi.h ../bmp.h ../ugiSDL.h ../sdlmain.h
../ugi.o: ../ugi.c ../ugi.h
../ugiSDL.o: ../ugiSDL.c ../ugi.h ../bmp.h
richedit.o: richedit.c richedit.h ../ugi.h
test.o: test.c ../ugi.h ../bmp.h ../ugiSDL.h ../sdlmain.h ../bold.xbm


.PHONY : clean

clean:
	-rm -f $(EXECUTABLE)
	-rm -f *.o