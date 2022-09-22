add_executable(win-tracer win-tracer.cpp
 $<TARGET_OBJECTS:utils> "compiler-config.cpp" "main.cpp")

target_link_libraries(win-tracer PRIVATE
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json
)

add_executable(win-tracer-test 
    win-tracer.cpp 
    compiler-config.cpp 
    win-tracer.test.cpp 
    )

target_link_libraries(win-tracer-test PRIVATE
    ${Boost_LIBRARIES}
    nlohmann_json nlohmann_json::nlohmann_json)


add_test(NAME win_test_tracer COMMAND win-tracer-test)