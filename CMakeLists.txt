
cmake_minimum_required(VERSION 3.16)
project("Matsuri" C)


set(CMAKE_C_STANDARD 11)


####
if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	set(MATSURI_CFLAGS /W4)
endif ()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
	set(MATSURI_CFLAGS -Wall -Wextra -Wconversion -pedantic)
endif ()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
	# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
endif ()
####


# Matsuri library
add_library("matsuri-library"     OBJECT "./source/matsuri-library.c")
add_library("matsuri-library-pic" OBJECT "./source/matsuri-library.c")

target_compile_options("matsuri-library"     PRIVATE ${MATSURI_CFLAGS})
target_compile_options("matsuri-library-pic" PRIVATE ${MATSURI_CFLAGS})

set_target_properties("matsuri-library" PROPERTIES C_VISIBILITY_PRESET hidden)
# set_target_properties("matsuri-library-pic" PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-library-pic" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


# Executable "generate-wavs"
add_executable("generate-wavs" "./source/generate-wavs.c")
target_compile_options("generate-wavs" PRIVATE ${MATSURI_CFLAGS})
target_link_libraries("generate-wavs" PRIVATE "matsuri-library")


# Plugin, CLAP
add_library("matsuri-clap" MODULE "./source/clap/plugin.c" "./source/voice-allocator.c")
target_compile_options("matsuri-clap" PRIVATE ${MATSURI_CFLAGS})
target_link_libraries("matsuri-clap" PRIVATE "matsuri-library-pic")

set_target_properties("matsuri-clap" PROPERTIES PREFIX "")
set_target_properties("matsuri-clap" PROPERTIES OUTPUT_NAME "matsuri")
set_target_properties("matsuri-clap" PROPERTIES SUFFIX ".clap")
set_target_properties("matsuri-clap" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
# set_target_properties("matsuri-clap" PROPERTIES C_VISIBILITY_PRESET hidden)


# Plugin, WASM
if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
	set(WASM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm/matsuri.wasm")
	set(WASM_SOURCES "${CMAKE_SOURCE_DIR}/source/wasm/plugin.c"
	                 "${CMAKE_SOURCE_DIR}/source/matsuri-library.c"
	                 "${CMAKE_SOURCE_DIR}/source/voice-allocator.c")

	set(WASM_DIR "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm")

	set(WASM_TEST_PAGE_SRC "${CMAKE_SOURCE_DIR}/source/wasm/test-page.html")
	set(WASM_TEST_PAGE_DEST "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm/test-page.html")

	set(WASM_JS_SRC "${CMAKE_SOURCE_DIR}/source/wasm/plugin.js")
	set(WASM_JS_DEST "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm/matsuri.js")

	set(WASM_JS_WORKLET_SRC "${CMAKE_SOURCE_DIR}/source/wasm/plugin-worklet.js")
	set(WASM_JS_WORKLET_DEST "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm/matsuri-worklet.js")

	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		set(MATSURI_WASM_CFLAGS -O3 -flto -DNDEBUG)
	endif ()

	add_custom_command(
		OUTPUT ${WASM_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${WASM_DIR}
	)

	add_custom_command(
		OUTPUT ${WASM_OUTPUT}
		COMMAND clang --target=wasm32 -DFREESTANDING -ffreestanding --no-standard-libraries
		        ${MATSURI_CFLAGS} ${MATSURI_WASM_CFLAGS} -fuse-ld=lld
		        -Wl,--entry=Initialise -Wl,--export=Initialise -Wl,--export=Midi -Wl,--export=Render
		        -Wl,--strip-all -Wl,--merge-data-segments -z stack-size=1000000
		        ${WASM_SOURCES} -o ${WASM_OUTPUT}
		DEPENDS ${WASM_SOURCES}
	)

	add_custom_command(
		OUTPUT ${WASM_TEST_PAGE_DEST}
		COMMAND ${CMAKE_COMMAND} -E copy ${WASM_TEST_PAGE_SRC} ${WASM_TEST_PAGE_DEST}
		DEPENDS ${WASM_TEST_PAGE_SRC}
	)

	add_custom_command(
		OUTPUT ${WASM_JS_DEST}
		COMMAND ${CMAKE_COMMAND} -E copy ${WASM_JS_SRC} ${WASM_JS_DEST}
		DEPENDS ${WASM_JS_SRC}
	)

	add_custom_command(
		OUTPUT ${WASM_JS_WORKLET_DEST}
		COMMAND ${CMAKE_COMMAND} -E copy ${WASM_JS_WORKLET_SRC} ${WASM_JS_WORKLET_DEST}
		DEPENDS ${WASM_JS_WORKLET_SRC}
	)

	add_custom_target("wasm" ALL DEPENDS ${WASM_DIR} ${WASM_OUTPUT})
	add_custom_target("wasm-test-page" ALL DEPENDS ${WASM_DIR} ${WASM_TEST_PAGE_DEST})
	add_custom_target("wasm-js-node" ALL DEPENDS ${WASM_DIR} ${WASM_JS_DEST})
	add_custom_target("wasm-js-worklet" ALL DEPENDS ${WASM_DIR} ${WASM_JS_WORKLET_DEST})
endif()


#
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	set_target_properties("matsuri-library"     PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("matsuri-library-pic" PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("generate-wavs"       PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("matsuri-clap"        PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()
