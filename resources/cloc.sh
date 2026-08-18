#!/bin/bash

FILES="
../source/clap/plugin.cpp \
../source/ui/ui.cpp \
../source/ui/yuika.cpp \
../source/wasm/plugin.c \
../source/matsuri.c \
../source/misc.c \
../source/voice-allocator.c"

cloc $FILES
