add_library(utils OBJECT
    checksum.cpp
    logger.cpp
    profiler.cpp
    path.cpp
)

# Use a seperate lib since it has additional dependencies.
add_library(archive OBJECT
    archive.cpp
)

add_executable(utils-test
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:archive>
    utils.test.cpp
)

target_link_libraries(utils-test PRIVATE
    ${LIBARCHIVE_LIBRARY}
    ${ZLIB_LIBRARIES}
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})

add_test(NAME test_utils COMMAND utils-test)
