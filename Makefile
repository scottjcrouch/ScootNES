CC= 	g++ -std=c++11

TARGET=	bin/ScootNES

OBJECTS= \
	build/APU.o \
	build/Cart.o \
	build/Console.o \
	build/Controller.o \
	build/CPU.o \
	build/main.o \
	build/PPU.o \
	build/Graphics.o \
	build/mappers/Mapper0.o \
	build/mappers/Mapper1.o \
	build/SoundQueue.o \
	build/nes_apu/apu_snapshot.o \
	build/nes_apu/Blip_Buffer.o \
	build/nes_apu/Multi_Buffer.o \
	build/nes_apu/Nes_Apu.o \
	build/nes_apu/Nes_Namco.o \
	build/nes_apu/Nes_Oscs.o \
	build/nes_apu/Nes_Vrc6.o \
	build/nes_apu/Nonlinear_Buffer.o

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

INC+= -Isrc -Isrc/boost -Isrc/nes_apu -MMD -MP

ifeq ($(OS),Windows_NT)
	TARGET=	bin/ScootNES.exe
	LFLAGS+= -lmingw32 -lSDL2main -lSDL2
	LIB+= -L lib/SDL2
	INC+= -I include/SDL2
else
	LFLAGS+= $(shell sdl2-config --libs)
	INC+= $(shell sdl2-config --cflags)
endif

.PHONY: all
all: subdirs $(TARGET)

.PHONY: debug
debug: CFLAGS = -g -O0
debug: all

subdirs:
	mkdir -p "build"
	mkdir -p "build/mappers"
	mkdir -p "build/nes_apu"
	mkdir -p "build/boost"

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $(TARGET) $(LIB) $(LFLAGS)

build/%.o: src/%.cpp
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

.PHONY: clean
clean:
	rm -r build
	rm $(TARGET)
