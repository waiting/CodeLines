cmake_minimum_required(VERSION 2.8)

project(CodeLines)

include_directories(main/include main/include/winux)

aux_source_directory(main/src MAIN_SRCS)
aux_source_directory(main/src/winux WINUX_SRCS)

add_executable(CodeLines ${MAIN_SRCS})

add_library(winux ${WINUX_SRCS})

target_link_libraries(CodeLines winux)

if(MSVC)

else()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

target_compile_options(winux PUBLIC -fPIC -ffunction-sections -fdata-sections)

# Remove "-rdynamic"
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

target_link_libraries(CodeLines pthread)
set_target_properties(CodeLines PROPERTIES LINK_FLAGS -Wl,--gc-sections)


endif(MSVC)
