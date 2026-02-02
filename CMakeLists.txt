cmake_minimum_required(VERSION 2.8)

project(CodeLines)

aux_source_directory(main/src/winux TMPWINUX_SRCS)
add_library(tmpwinux_a STATIC ${TMPWINUX_SRCS})
target_include_directories(tmpwinux_a PUBLIC main/include/winux)
if(UNIX)
    target_compile_options(tmpwinux_a PRIVATE -ffunction-sections -fdata-sections)
endif()
#target_link_libraries(tmpwinux_a PRIVATE dl)
set_property(TARGET tmpwinux_a PROPERTY POSITION_INDEPENDENT_CODE ON)

aux_source_directory(main/src MAIN_SRCS)
add_executable(CodeLines ${MAIN_SRCS})
target_include_directories(CodeLines PRIVATE main/include)
target_link_libraries(CodeLines PRIVATE tmpwinux_a)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    # Remove "-rdynamic"
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

    target_link_libraries(CodeLines PRIVATE pthread)
    if(CMAKE_VERSION VERSION_LESS 3.13)
        set_target_properties(CodeLines PROPERTIES LINK_FLAGS -Wl,--gc-sections)
        #set_target_properties(CodeLines PROPERTIES LINK_FLAGS -Wl,--gc-sections)
        set_property(TARGET CodeLines PROPERTY LINK_FLAGS -Wl,--gc-sections)
    else()
        target_link_options(CodeLines PRIVATE "LINKER:--gc-sections")
    endif()
endif()
