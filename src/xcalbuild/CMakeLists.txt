add_library(command-line OBJECT
    command-line.cpp
)

add_executable(xcalbuild
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:archive>
    $<TARGET_OBJECTS:tracer>
    $<TARGET_OBJECTS:prober>
    $<TARGET_OBJECTS:work-item-processor>
    $<TARGET_OBJECTS:build-processor>
    $<TARGET_OBJECTS:toolchain-profile>
    $<TARGET_OBJECTS:command-line>
    main.cpp
)

target_link_libraries(xcalbuild PRIVATE
    ${LIBARCHIVE_LIBRARY}
    ${ZLIB_LIBRARIES}
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(command-line-test
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:command-line>
    command-line.test.cpp
)

target_link_libraries(command-line-test PRIVATE
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})

add_test(NAME test_command_line COMMAND command-line-test)
