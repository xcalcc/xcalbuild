add_library(prober OBJECT
    prober.cpp
)

add_executable(prober-test
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:prober>
    $<TARGET_OBJECTS:toolchain-profile>
    prober.test.cpp
)

target_link_libraries(prober-test PRIVATE
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT})

add_test(NAME test_prober COMMAND prober-test)
