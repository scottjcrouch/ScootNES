CC= 	g++ -std=c++11

TARGET=	bin/basicnes

OBJECTS= \
	build/APU.o \
	build/Cart.o \
	build/Console.o \
	build/Controller.o \
	build/CPU.o \
	build/main.o \
	build/PPU.o \
	build/Graphics.o \
	build/Mapper.o \
	build/Mapper0.o

DEPS= $(OBJECTS:.o=.d)
-include $(DEPS)

# -w, suppress warnings
# -g, debug symbols (disable -O2 beforehand)
# -Wall, enable most warnings
# -Wl,-subsystem,windows gets rid of the console window
# -mwindows or -mconsole do similar
# -O2, optimize
# -MMD -MP, generate dependency (.d) files with phony targets
CFLAGS=	-O3 -DNDEBUG

INC+= -Isrc -MMD -MP

ifeq ($(OS),Windows_NT)
	TARGET=	bin/basicnes.exe
	LFLAGS+= -lmingw32 -lSDL2main -lSDL2
	LIB+= -L lib/SDL2
	INC+= -I include/SDL2
else
	LFLAGS+= $(shell sdl2-config --libs)
	INC+= $(shell sdl2-config --cflags)
endif

.PHONY: all
all: $(TARGET)

.PHONY: debug
debug: CFLAGS = -g -Og
debug: all

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $(TARGET) $(LIB) $(LFLAGS)

build/%.o: src/%.cpp
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<


.PHONY: clean
clean:
	rm -r build/*
	rm $(TARGET)
