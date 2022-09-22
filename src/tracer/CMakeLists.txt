add_library(tracer OBJECT
    tracer.cpp
)

add_executable(tracer-test
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:command-line>
    $<TARGET_OBJECTS:toolchain-profile>
    $<TARGET_OBJECTS:tracer>
    tracer.test.cpp
)

target_link_libraries(tracer-test PRIVATE
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT})

add_test(NAME test_tracer COMMAND tracer-test)
