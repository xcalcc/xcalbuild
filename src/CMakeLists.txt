configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)

if (UNIX)
    add_subdirectory(linux)
endif (UNIX)

if (WIN32)
    add_subdirectory(windows)
endif (WIN32)

include_directories(SYSTEM
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

include_directories(PRIVATE
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/prober
    ${CMAKE_SOURCE_DIR}/src/tracer
    ${CMAKE_SOURCE_DIR}/src/build-processor
    ${CMAKE_SOURCE_DIR}/src/work-item-processor
    ${CMAKE_SOURCE_DIR}/src/profile
    ${CMAKE_SOURCE_DIR}/src/xcalbuild
)

add_subdirectory(utils)
add_subdirectory(profile)
add_subdirectory(prober)
add_subdirectory(tracer)
add_subdirectory(work-item-processor)
add_subdirectory(build-processor)
add_subdirectory(xcalbuild)