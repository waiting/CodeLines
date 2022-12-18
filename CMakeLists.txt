cmake_minimum_required(VERSION 2.8)

project(CodeLines)

include_directories(main/include main/include/winux)

aux_source_directory(main/src MAIN_SRCS)
aux_source_directory(main/src/winux WINUX_SRCS)
add_executable(CodeLines ${MAIN_SRCS} ${WINUX_SRCS})

if(MSVC)

else()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
target_link_libraries(CodeLines dl pthread)

endif(MSVC)
