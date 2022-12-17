cmake_minimum_required(VERSION 3.0)

project(CodeLines)

include_directories(main/include main/include/winux)

aux_source_directory(main/src MAIN_SRCS)
aux_source_directory(main/src/winux WINUX_SRCS)
add_executable(CodeLines ${MAIN_SRCS} ${WINUX_SRCS})
