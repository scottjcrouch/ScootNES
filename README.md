# ScootNES

![Super Mario Bros. (1985)](https://i.imgur.com/F02uO7Y.png)
![Excitebike (1984)](https://i.imgur.com/U0KrFyt.png)

A basic NTSC NES emulator in C++.

Support is currently limited to mapper 0 (NROM) and 1 (MMC1) games. See [here](http://tuxnes.sourceforge.net/nesmapper.txt) for a list of mappers used by most ROMs.

## Requirements
* GCC version supporting C++11
* SLD2

## Building
On debian derivatives:

    sudo apt-get install libsdl2-dev
    make all

## Running
    cd bin
    ./ScootNES path_to_rom.nes

## Building on Windows
ScootNES can be built on Windows with Mingw-w64 and MSYS binaries added to %PATH%. SDL2 development library and header files for Mingw 64-bit ([found here](https://www.libsdl.org/download-2.0.php)) must be copied to lib/SDL2 and include/SDL2 in the project folder respectively, as well as placing the corresponding SDL2.dll in the executable's directory before running.

## Controls
* D-pad: Arrow keys
* A: Z
* B: X
* Start: Return
* Select: Spacebar
* Quit: Escape
* Pause: P

## Credits

* Blargg's NES 2A03 APU sound chip emulator library
* The NesDev.com community
