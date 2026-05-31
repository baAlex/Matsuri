
cmake_minimum_required(VERSION 3.16)
project("Research" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
	set(RESEARCH_FLAGS -Wall -Wextra -Wconversion -pedantic)

	if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
		set(RESEARCH_FLAGS ${RESEARCH_FLAGS} -fsanitize=undefined)
		set(RESEARCH_LINK_FLAGS -fsanitize=undefined)
	endif ()
endif ()


add_executable(test-ui "test-ui.cpp" "ui.cpp" "canvas.cpp" "misc.cpp")
add_executable(test-canvas "test-canvas.cpp" "canvas.cpp" "misc.cpp")

target_compile_options("test-ui" PRIVATE ${RESEARCH_FLAGS})
target_compile_options("test-canvas" PRIVATE ${RESEARCH_FLAGS})
target_link_options("test-ui" PRIVATE ${RESEARCH_LINK_FLAGS})
target_link_options("test-canvas" PRIVATE ${RESEARCH_LINK_FLAGS})
