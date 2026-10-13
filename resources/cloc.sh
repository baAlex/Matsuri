#!/bin/bash

CXX_FILES="
../source/clap/plugin.cpp \
../source/ui/ui-backend.cpp \
../source/ui/ui-frontend.cpp \
../source/ui/yuika.cpp"

C_FILES="
../source/lv2/plugin.c \
../source/wasm/plugin.c \
../source/matsuri.c \
../source/misc.c \
../source/voice-allocator.c"

cloc $CXX_FILES
cloc $C_FILES
