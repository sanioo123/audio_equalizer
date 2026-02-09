#!/bin/bash

set -e

echo "Compilando AudioEqualizer..."

cd "$(dirname "$0")"

g++ -std=c++17 -O2 -DNDEBUG -DUNICODE -D_UNICODE \
    -I../external/imgui -I../external/imgui/backends -I../src -I../external/miniaudio \
    ../src/main.cpp \
    ../src/audio/*.cpp \
    ../src/dsp/*.cpp \
    ../src/gui/*.cpp \
    ../external/imgui/imgui*.cpp \
    ../external/imgui/backends/imgui_impl_win32.cpp \
    ../external/imgui/backends/imgui_impl_dx11.cpp \
    -o AudioEqualizer.exe \
    -static -static-libgcc -static-libstdc++ -mwindows \
    -ld3d11 -ldxgi -ld3dcompiler -lole32 -luuid -ldwmapi

echo "Compilación exitosa: AudioEqualizer.exe"
