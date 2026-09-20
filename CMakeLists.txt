
cmake_minimum_required(VERSION 3.16)
project("Matsuri" C CXX)


set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)


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
add_library("matsuri-library"     OBJECT "./source/matsuri.c")
add_library("matsuri-library-pic" OBJECT "./source/matsuri.c")

target_compile_options("matsuri-library"     PRIVATE ${MATSURI_CFLAGS})
target_compile_options("matsuri-library-pic" PRIVATE ${MATSURI_CFLAGS})

set_target_properties("matsuri-library"     PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-library-pic" PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-library-pic" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


# UI library
set(UI_LIBRARY_SOURCES "./source/ui/ui-backend.cpp"
                       "./source/ui/ui-frontend.cpp"
                       "./source/ui/yuika.cpp"
)

add_library("ui-library"     OBJECT ${UI_LIBRARY_SOURCES})
add_library("ui-library-pic" OBJECT ${UI_LIBRARY_SOURCES})

target_compile_options("ui-library"     PRIVATE ${MATSURI_CFLAGS})
target_compile_options("ui-library-pic" PRIVATE ${MATSURI_CFLAGS})

set_target_properties("ui-library"     PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties("ui-library-pic" PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties("ui-library-pic" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

if (UNIX) # ???, I want to assume that X11 doesn't exist outside unixes
	target_compile_definitions("ui-library"     PRIVATE MATSURI_UI=1)
	target_compile_definitions("ui-library-pic" PRIVATE MATSURI_UI=1)
	target_link_libraries("ui-library"          PRIVATE "X11")
	target_link_libraries("ui-library-pic"      PRIVATE "X11")
elseif (WIN32)
	target_compile_definitions("ui-library"     PRIVATE MATSURI_UI=2)
	target_compile_definitions("ui-library-pic" PRIVATE MATSURI_UI=2)
endif ()

find_package(SDL3)
if(SDL3_FOUND)
	set(TEST_CFLAGS "")

	add_executable("yuika-test" "./source/ui/yuika-test.cpp")

	target_compile_options("yuika-test" PRIVATE ${MATSURI_CFLAGS} ${TEST_CFLAGS})
	target_link_libraries("yuika-test" PRIVATE "ui-library" SDL3::SDL3)

	if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
		# Can be noisy:
		target_link_options("yuika-test" PRIVATE -fsanitize=address,undefined)
		set(TEST_CFLAGS ${TEST_CFLAGS} -fsanitize=address,undefined)
	endif ()

	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		set_target_properties("yuika-test" PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif ()
endif ()


# Executable "generate-wavs"
add_executable("generate-wavs" "./source/generate-wavs.c")

target_compile_options("generate-wavs" PRIVATE ${MATSURI_CFLAGS})
target_link_libraries("generate-wavs" PRIVATE "matsuri-library")


# Executable "generate-font-atlas"
find_package(Freetype)
if (Freetype_FOUND)
	add_executable("generate-font-atlas" "./source/ui/generate-font-atlas.cpp")
	target_link_libraries("generate-font-atlas" PRIVATE Freetype::Freetype)
endif ()


# Plugin, CLAP
set(CLAP_SOURCES "./source/clap/plugin.cpp"
                 "./source/voice-allocator.c"
                 "./source/misc.c"
)

add_library("matsuri-clap" MODULE ${CLAP_SOURCES})
target_compile_options("matsuri-clap" PRIVATE ${MATSURI_CFLAGS})
target_link_libraries("matsuri-clap" PRIVATE "matsuri-library-pic" "ui-library-pic")

target_include_directories("matsuri-clap" PRIVATE "./source/thirdparty/clap/include")

set_target_properties("matsuri-clap" PROPERTIES PREFIX "")
set_target_properties("matsuri-clap" PROPERTIES OUTPUT_NAME "matsuri")
set_target_properties("matsuri-clap" PROPERTIES SUFFIX ".clap")
set_target_properties("matsuri-clap" PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-clap" PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties("matsuri-clap" PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_target_properties("matsuri-clap" PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/matsuri-clap")

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
	target_link_options("matsuri-clap" PRIVATE -Wl,--no-undefined) # To not forget the 'extern "C"'s
endif ()

if (UNIX)
	target_compile_definitions("matsuri-clap" PRIVATE MATSURI_UI=1)
elseif (WIN32)
	target_compile_definitions("matsuri-clap" PRIVATE MATSURI_UI=2)

	if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
		target_link_options("matsuri-clap" PRIVATE -static) # (btw, MSVC doesn't need this)
	endif ()
endif ()


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

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
	target_link_options("matsuri-lv2" PRIVATE -Wl,--no-undefined)
endif ()

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
	                 "${CMAKE_SOURCE_DIR}/source/matsuri.c"
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
	set_target_properties("ui-library"          PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("ui-library-pic"      PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("generate-wavs"       PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("matsuri-clap"        PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	set_target_properties("matsuri-lv2"         PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif ()
