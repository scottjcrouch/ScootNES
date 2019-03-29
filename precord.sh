#!/bin/bash

cd bin
perf record -g -m 64 ./ScootNES ~/Downloads/roms/supermariobros.nes
