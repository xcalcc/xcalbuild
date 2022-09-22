add_library(work-item-processor OBJECT
    work-item-processor.cpp
)

add_executable(work-item-processor-test
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:archive>
    $<TARGET_OBJECTS:prober>
    $<TARGET_OBJECTS:build-processor>
    $<TARGET_OBJECTS:toolchain-profile>
    $<TARGET_OBJECTS:command-line>
    $<TARGET_OBJECTS:work-item-processor>
    work-item-processor.test.cpp
)

target_link_libraries(work-item-processor-test PRIVATE
    ${LIBARCHIVE_LIBRARY}
    ${ZLIB_LIBRARIES}
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT})

add_test(NAME test_work_item_processor COMMAND work-item-processor-test)
