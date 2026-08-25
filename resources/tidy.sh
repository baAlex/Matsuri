#!/bin/bash

CXX_FILES="
../source/clap/plugin.cpp \
../source/ui/ui-backend.cpp \
../source/ui/ui-frontend.cpp \
../source/ui/yuika.cpp"

C_FILES="
../source/wasm/plugin.c \
../source/matsuri.c \
../source/misc.c \
../source/voice-allocator.c"

CXX_FLAGS="-std=c++17 -Wall -Wextra -Wconversion -pedantic -Wold-style-cast -I../source/ -I../source/thirdparty/clap/include"
C_FLAGS="-std=c17 -Wall -Wextra -Wconversion -pedantic -I../source/ -I../source/thirdparty/clap/include"

clang-tidy $CXX_FILES -- $CXX_FLAGS
clang-tidy $C_FILES -- $C_FLAGS
