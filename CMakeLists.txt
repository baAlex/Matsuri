
cmake_minimum_required(VERSION 3.16)
project("Matsuri" C)


set(CMAKE_C_STANDARD 99)


####
if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	set(MATSURI_CFLAGS /W4)
endif ()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
	set(MATSURI_CFLAGS -Wall -Wextra -Wconversion -pedantic)
endif ()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
endif ()
####


# Matsuri library
add_library("matsuri-library"     OBJECT "./source/matsuri-library.c")
add_library("matsuri-library-pic" OBJECT "./source/matsuri-library.c")

target_compile_options("matsuri-library"     PRIVATE ${MATSURI_CFLAGS})
target_compile_options("matsuri-library-pic" PRIVATE ${MATSURI_CFLAGS})

set_target_properties("matsuri-library-pic" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


# Executable "generate-wavs"
add_executable("generate-wavs" "./source/generate-wavs.c")
target_compile_options("generate-wavs" PRIVATE ${MATSURI_CFLAGS})
target_link_libraries("generate-wavs" PRIVATE "matsuri-library")


# Plugin, WASM
if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
	set(WASM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/matsuri.wasm")
	set(WASM_SOURCES "${CMAKE_SOURCE_DIR}/source/wasm/plugin.c"
	                 "${CMAKE_SOURCE_DIR}/source/matsuri-library.c")

	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		set(MATSURI_WASM_CFLAGS "-O3")
	endif ()

	add_custom_command(
		OUTPUT ${WASM_OUTPUT}
		COMMAND clang --target=wasm32 -DFREESTANDING -ffreestanding --no-standard-libraries
		        ${MATSURI_CFLAGS} ${MATSURI_WASM_CFLAGS} -fuse-ld=lld
		        -Wl,--entry=Initialise -Wl,--export=Initialise -Wl,--export=Render
		        -Wl,--strip-all -Wl,--merge-data-segments
		        ${WASM_SOURCES} -o ${WASM_OUTPUT}
		DEPENDS ${WASM_SOURCES}
	)

	add_custom_target("matsuri-wasm" ALL DEPENDS ${WASM_OUTPUT})
endif()


#
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	set_target_properties("matsuri-library"     PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("matsuri-library-pic" PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("generate-wavs"       PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()
