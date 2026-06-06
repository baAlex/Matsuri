
cmake_minimum_required(VERSION 3.16)
project("Matsuri" C CXX)


set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)


####
if (MSVC)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_options(/experimental:c11atomics /Gy /Gw)
	set(MATSURI_CFLAGS /W4)
endif ()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
	set(MATSURI_CFLAGS -Wall -Wextra -Wconversion -pedantic -ffunction-sections -fdata-sections)
endif ()
####


# Matsuri library
add_library("matsuri-library"     OBJECT "./source/matsuri-library.c")
add_library("matsuri-library-pic" OBJECT "./source/matsuri-library.c")

target_compile_options("matsuri-library"     PRIVATE ${MATSURI_CFLAGS})
target_compile_options("matsuri-library-pic" PRIVATE ${MATSURI_CFLAGS})

set_target_properties("matsuri-library"     PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-library-pic" PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-library-pic" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


# Executable "generate-wavs"
add_executable("generate-wavs" "./source/generate-wavs.c")

target_compile_options("generate-wavs" PRIVATE ${MATSURI_CFLAGS})
target_link_libraries("generate-wavs" PRIVATE "matsuri-library")


# Freetype library
set(FREETYPE_SOURCES "./source/thirdparty/freetype/src/base/ftbase.c"
                     "./source/thirdparty/freetype/src/base/ftbbox.c"
                     "./source/thirdparty/freetype/src/base/ftbdf.c"
                     "./source/thirdparty/freetype/src/base/ftbitmap.c"
                     "./source/thirdparty/freetype/src/base/ftcid.c"
                     "./source/thirdparty/freetype/src/base/ftdebug.c"
                     "./source/thirdparty/freetype/src/base/ftfstype.c"
                     "./source/thirdparty/freetype/src/base/ftgasp.c"
                     "./source/thirdparty/freetype/src/base/ftglyph.c"
                     "./source/thirdparty/freetype/src/base/ftgxval.c"
                     "./source/thirdparty/freetype/src/base/ftinit.c"
                     "./source/thirdparty/freetype/src/base/ftmm.c"
                     "./source/thirdparty/freetype/src/base/ftotval.c"
                     "./source/thirdparty/freetype/src/base/ftpatent.c"
                     "./source/thirdparty/freetype/src/base/ftpfr.c"
                     "./source/thirdparty/freetype/src/base/ftstroke.c"
                     "./source/thirdparty/freetype/src/base/ftsynth.c"
                     "./source/thirdparty/freetype/src/base/fttype1.c"
                     "./source/thirdparty/freetype/src/base/ftwinfnt.c"

                     # Configured modules:
                     "./source/thirdparty/freetype/src/autofit/autofit.c"
                     "./source/thirdparty/freetype/src/cff/cff.c"
                     "./source/thirdparty/freetype/src/raster/raster.c"
                     "./source/thirdparty/freetype/src/smooth/smooth.c"
                     "./source/thirdparty/freetype/src/truetype/truetype.c"
                     "./source/thirdparty/freetype/src/sfnt/sfnt.c"
                     "./source/thirdparty/freetype/src/psaux/psaux.c"
                     "./source/thirdparty/freetype/src/psnames/psnames.c"

                     # Doesn't break the compilation and doesn't throw runtime
                     # errors, but it if absent, it breaks glyph sizes in a
                     # disgusting manner:
                     "./source/thirdparty/freetype/src/pshinter/pshinter.c"

                     # In case something is missing:
                     #"./source/thirdparty/freetype/src/svg/svg.c"
                     #"./source/thirdparty/freetype/src/sdf/sdf.c"
                     #"./source/thirdparty/freetype/src/bdf/bdf.c"
                     #"./source/thirdparty/freetype/src/winfonts/winfnt.c"
                     #"./source/thirdparty/freetype/src/pcf/pcf.c"
                     #"./source/thirdparty/freetype/src/cid/type1cid.c"
                     #"./source/thirdparty/freetype/src/hvf/hvf.c"
                     #"./source/thirdparty/freetype/src/pfr/pfr.c"
                     #"./source/thirdparty/freetype/src/type1/type1.c"
                     #"./source/thirdparty/freetype/src/type42/type42.c"
                     #"./source/thirdparty/freetype/src/gzip/ftgzip.c"
                     #"./source/thirdparty/freetype/src/lzw/ftlzw.c"
)

if (WIN32)
	list(APPEND FREETYPE_SOURCES "./source/thirdparty/freetype/builds/windows/ftsystem.c")
elseif (UNIX)
	list(APPEND FREETYPE_SOURCES "./source/thirdparty/freetype/builds/unix/ftsystem.c")
else ()
	# This one is ANSI C:
	# (previous two are doing some sort of mmap trick to speed up file readings)
	list(APPEND FREETYPE_SOURCES "./source/thirdparty/freetype/src/base/ftsystem.c")
endif ()

add_library("freetype-library-pic" OBJECT ${FREETYPE_SOURCES})

target_compile_definitions("freetype-library-pic" PRIVATE
	FT2_BUILD_LIBRARY # Needed
	FT_CONFIG_MODULES_H="${CMAKE_SOURCE_DIR}/source/thirdparty/freetype-custom/ftmodule.h"
	FT_CONFIG_OPTIONS_H="${CMAKE_SOURCE_DIR}/source/thirdparty/freetype-custom/ftoption.h")

set_target_properties("freetype-library-pic" PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("freetype-library-pic" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories("freetype-library-pic" PRIVATE "./source/thirdparty/freetype/include")

if (UNIX)
	# It's 2026, it's illegal to a non-POSIX UNIX (ewww... all caps acronyms) to even exist
	target_compile_definitions("freetype-library-pic" PRIVATE HAVE_UNISTD_H)
	target_compile_definitions("freetype-library-pic" PRIVATE HAVE_FCNTL_H)
endif ()


# Research stuff
add_executable(test-ui "./source/research/test-ui.cpp" "./source/research/ui.cpp" "./source/research/error.cpp" "./source/research/canvas.cpp" "./source/research/shared.cpp")
add_executable(test-canvas "./source/research/test-canvas.cpp" "./source/research/error.cpp" "./source/research/canvas.cpp" "./source/research/shared.cpp")
add_executable(test-metrics "./source/research/test-metrics.cpp")

target_compile_options("test-ui" PRIVATE ${MATSURI_CFLAGS})
target_compile_options("test-canvas" PRIVATE ${MATSURI_CFLAGS})
target_compile_options("test-metrics" PRIVATE ${MATSURI_CFLAGS})

target_link_libraries("test-ui" PRIVATE "freetype-library-pic")
target_link_libraries("test-canvas" PRIVATE "freetype-library-pic")
target_link_libraries("test-metrics" PRIVATE "freetype-library-pic")

target_include_directories("test-ui" PRIVATE "./source/thirdparty/freetype/include")
target_include_directories("test-canvas" PRIVATE "./source/thirdparty/freetype/include")
target_include_directories("test-metrics" PRIVATE "./source/thirdparty/freetype/include")

if (MSVC)
	# Freetype is chonky
	target_link_options("test-canvas" PRIVATE /OPT:REF /OPT:ICF)
	target_link_options("test-ui" PRIVATE /OPT:REF /OPT:ICF)
	target_link_options("test-metrics" PRIVATE /OPT:REF /OPT:ICF)
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
	target_link_options("test-canvas" PRIVATE -Wl,--gc-sections)
	target_link_options("test-ui" PRIVATE -Wl,--gc-sections)
	target_link_options("test-metrics" PRIVATE -Wl,--gc-sections)
endif ()

find_package(Freetype)

if (Freetype_FOUND)
	add_executable(test-metrics-system "./source/research/test-metrics.cpp")
	target_compile_options("test-metrics-system" PRIVATE ${MATSURI_CFLAGS})
	target_link_libraries("test-metrics-system" PRIVATE Freetype::Freetype)
endif ()


# Plugin, CLAP
set(CLAP_SOURCES "./source/clap/plugin.c"
                 "./source/voice-allocator.c"
                 "./source/misc.c"
)

add_library("matsuri-clap" MODULE ${CLAP_SOURCES})
target_compile_options("matsuri-clap" PRIVATE ${MATSURI_CFLAGS})
target_link_libraries("matsuri-clap" PRIVATE "matsuri-library-pic")

target_include_directories("matsuri-clap" PRIVATE "./source/thirdparty/clap/include")

set_target_properties("matsuri-clap" PROPERTIES PREFIX "")
set_target_properties("matsuri-clap" PROPERTIES OUTPUT_NAME "matsuri")
set_target_properties("matsuri-clap" PROPERTIES SUFFIX ".clap")
set_target_properties("matsuri-clap" PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-clap" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_target_properties("matsuri-clap" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/matsuri-clap")


# Plugin, LV2
set(LV2_SOURCES "./source/lv2/plugin.c"
                "./source/voice-allocator.c"
                "./source/misc.c"
)

add_library("matsuri-lv2" MODULE ${LV2_SOURCES})
target_compile_options("matsuri-lv2" PRIVATE ${MATSURI_CFLAGS})
target_link_libraries("matsuri-lv2" PRIVATE "matsuri-library-pic")

target_include_directories("matsuri-lv2" PRIVATE "./source/thirdparty/lv2/include")

set_target_properties("matsuri-lv2" PROPERTIES PREFIX "")
set_target_properties("matsuri-lv2" PROPERTIES OUTPUT_NAME "matsuri")
set_target_properties("matsuri-lv2" PROPERTIES SUFFIX ".lv2")
set_target_properties("matsuri-lv2" PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-lv2" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_target_properties("matsuri-lv2" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/matsuri-lv2")


set(LV2_DIR "${CMAKE_CURRENT_BINARY_DIR}/matsuri-lv2")
set(LV2_MANIFEST_SRC "${CMAKE_SOURCE_DIR}/source/lv2/manifest.ttl")
set(LV2_MANIFEST_DEST "${CMAKE_CURRENT_BINARY_DIR}/matsuri-lv2/manifest.ttl")

set(LV2_TTL_SRC "${CMAKE_SOURCE_DIR}/source/lv2/matsuri.ttl")
set(LV2_TTL_DEST "${CMAKE_CURRENT_BINARY_DIR}/matsuri-lv2/matsuri.ttl")

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/matsuri-lv2"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/matsuri-lv2"
)

add_custom_command(
	OUTPUT ${LV2_MANIFEST_DEST}
	COMMAND ${CMAKE_COMMAND} -E copy ${LV2_MANIFEST_SRC} ${LV2_MANIFEST_DEST}
	DEPENDS ${LV2_MANIFEST_SRC}
)

add_custom_command(
	OUTPUT ${LV2_TTL_DEST}
	COMMAND ${CMAKE_COMMAND} -E copy ${LV2_TTL_SRC} ${LV2_TTL_DEST}
	DEPENDS ${LV2_TTL_SRC}
)

add_custom_target("lv2-manifest" ALL DEPENDS ${LV2_DIR} ${LV2_MANIFEST_DEST})
add_custom_target("lv2-ttl" ALL DEPENDS ${LV2_DIR} ${LV2_TTL_DEST})


# Plugin, WASM
if (WASM AND "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")

	message(STATUS "WASM32 build set")

	set(WASM_SOURCES "${CMAKE_SOURCE_DIR}/source/wasm/plugin.c"
	                 "${CMAKE_SOURCE_DIR}/source/matsuri-library.c"
	                 "${CMAKE_SOURCE_DIR}/source/voice-allocator.c"
	                 "${CMAKE_SOURCE_DIR}/source/misc.c"
	)

	set(WASM_DIR "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm")
	set(WASM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm/matsuri.wasm")

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
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/matsuri-wasm"
	)

	add_custom_command(
		OUTPUT ${WASM_OUTPUT}
		COMMAND clang --target=wasm32 -DFREESTANDING -ffreestanding --no-standard-libraries
		        ${MATSURI_CFLAGS} ${MATSURI_WASM_CFLAGS} -fuse-ld=lld
		        -Wl,--entry=Initialise -Wl,--export=Initialise -Wl,--export=Midi -Wl,--export=Render
		        -Wl,--export=GetId -Wl,--export=GetName -Wl,--export=GetVendor -Wl,--export=GetUrl
		        -Wl,--export=GetDescription -Wl,--export=GetVersionString -Wl,--export=GetVersionMajor
		        -Wl,--export=GetVersionMinor -Wl,--export=GetCopyright
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
endif ()


#
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	set_target_properties("matsuri-library"     PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("matsuri-library-pic" PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("generate-wavs"       PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("matsuri-clap"        PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("matsuri-lv2"         PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()
