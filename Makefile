CC= 	g++

TARGET=	bin/basicnes

OBJECTS= \
	build/APU.o \
	build/Cart.o \
	build/Console.o \
	build/Controller.o \
	build/CPU.o \
	build/main.o \
	build/PPU.o \
	build/Renderer.o

# -w, suppress warnings
# -g, debug symbols (disable -O2 beforehand)
# -Wall, enable most warnings
# -Wl,-subsystem,windows gets rid of the console window
# -mwindows or -mconsole do similar
# -O2, optimize
CFLAGS=	-g

ifeq ($(OS),Windows_NT)
	TARGET=	bin/basicnes.exe
	LFLAGS+= -lmingw32
	LIB+= -L lib/SDL2-2.0.4
	INC+= -I include/SDL2-2.0.4
endif

LFLAGS+= -lSDL2main -lSDL2

INC+= -I src


.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $(TARGET) $(LIB) $(LFLAGS)

build/%.o: src/%.cpp
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<


.PHONY: clean
clean:
	rm -r build/*
	rm $(TARGET)
