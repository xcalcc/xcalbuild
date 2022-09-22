add_library(build-processor OBJECT
    build-processor.cpp
)

add_executable(build-processor-test
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:archive>
    $<TARGET_OBJECTS:prober>
    $<TARGET_OBJECTS:build-processor>
    $<TARGET_OBJECTS:toolchain-profile>
    $<TARGET_OBJECTS:command-line>
    $<TARGET_OBJECTS:work-item-processor>
    build-processor.test.cpp
)

target_link_libraries(build-processor-test PRIVATE
    ${LIBARCHIVE_LIBRARY}
    ${ZLIB_LIBRARIES}
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT})

add_test(NAME test_build_processor COMMAND build-processor-test)
