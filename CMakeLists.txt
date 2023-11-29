cmake_minimum_required(VERSION 2.8)

project(CodeLines)

aux_source_directory(main/src/winux winux_SRCS)
add_library(winux STATIC ${winux_SRCS})
target_include_directories(winux PUBLIC main/include/winux)
if(UNIX)
    target_compile_options(winux PRIVATE -ffunction-sections -fdata-sections)
endif()

aux_source_directory(main/src main_SRCS)
add_executable(CodeLines ${main_SRCS})
target_include_directories(CodeLines PRIVATE main/include)
target_link_libraries(CodeLines PRIVATE winux)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    # Remove "-rdynamic"
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

    target_link_libraries(CodeLines PRIVATE pthread)
    if(CMAKE_VERSION VERSION_LESS 3.13)
        set_target_properties(CodeLines PROPERTIES LINK_FLAGS -Wl,--gc-sections)
    else()
        target_link_options(CodeLines PRIVATE "LINKER:--gc-sections")
    endif()
endif()
