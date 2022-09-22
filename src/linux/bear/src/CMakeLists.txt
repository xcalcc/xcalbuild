# This file is distributed under MIT-LICENSE. See COPYING for details.

add_definitions(-D_GNU_SOURCE)

set(DEFAULT_PRELOAD_LIB libtracer-preload.so)
set(DEFAULT_PRELOAD_PATH ${CMAKE_INSTALL_PREFIX}/lib32:${CMAKE_INSTALL_PREFIX}/lib64)
set(DEFAULT_OUTPUT_FILE compile_commands.json)
set(DEFAULT_COMPILERS ar,cc,ld,gcc,llvm-gcc,clang,c++,g++,llvm-g++,clang++,arm-none-eabi-gcc,arm-none-eabi-g++)

add_library(tracer-preload64 SHARED
    stringarray.c
    protocol.c
    environ.c
    execs.c)
target_link_libraries(tracer-preload64 ${CMAKE_DL_LIBS})
set_target_properties(tracer-preload64 PROPERTIES
    COMPILE_DEFINITIONS "CLIENT")

add_library(tracer-preload32 SHARED
    stringarray.c
    protocol.c
    environ.c
    execs.c)
target_link_libraries(tracer-preload32 ${CMAKE_DL_LIBS})
set_target_properties(tracer-preload32 PROPERTIES
    COMPILE_DEFINITIONS "CLIENT")
set_target_properties(tracer-preload32 PROPERTIES COMPILE_FLAGS "-m32")

if(APPLE)
    # Macs have an entirely different linker.
    set_target_properties(tracer-preload64 PROPERTIES
        LINK_FLAGS "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/AppleExportedSymbolsList")
    set_target_properties(tracer-preload32 PROPERTIES
        LINK_FLAGS "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/AppleExportedSymbolsList -m32")
else()
    set_target_properties(tracer-preload64 PROPERTIES
        LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ExportedSymbolsList")
    set_target_properties(tracer-preload32 PROPERTIES
        LINK_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ExportedSymbolsList -m32")
endif()

add_executable(unix-tracer
    stringarray.c
    protocol.c
    main.c
    output.c
    json.c)
set_target_properties(unix-tracer PROPERTIES
    COMPILE_DEFINITIONS "SERVER")

include(CheckFunctionExists)
check_function_exists(vfork HAVE_VFORK)
check_function_exists(execve HAVE_EXECVE)
check_function_exists(execv HAVE_EXECV)
check_function_exists(execvpe HAVE_EXECVPE)
check_function_exists(execvp HAVE_EXECVP)
check_function_exists(execvP HAVE_EXECVP2)
check_function_exists(execl HAVE_EXECL)
check_function_exists(execlp HAVE_EXECLP)
check_function_exists(execle HAVE_EXECLE)
check_function_exists(posix_spawn HAVE_POSIX_SPAWN)
check_function_exists(posix_spawnp HAVE_POSIX_SPAWNP)
include(CheckSymbolExists)
check_symbol_exists(_NSGetEnviron crt_externs.h NEED_NSGETENVIRON)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
